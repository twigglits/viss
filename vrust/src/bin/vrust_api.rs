use std::net::SocketAddr;

use axum::{
    extract::{Path, Query, State},
    http::StatusCode,
    response::IntoResponse,
    routing::{get, post},
    Json, Router,
};
use anyhow::Context;
use serde::{Deserialize, Serialize};
use serde_json::json;

use postgres::{Client, NoTls};

use vrust::calibration::beta0_from_r0;
use vrust::io::age_pyramid_pg::load_age_pyramid_5yr_pg;
use vrust::io::contact_synth::synthetic_contact_matrix;
use vrust::model::seirs::{SeirsConfig, SeirsModel, SeirsState};

#[derive(Clone)]
struct AppState {
    pg_conn_str: String,
}

fn redact_conn_str(s: &str) -> String {
    // Avoid logging secrets in errors. Best-effort redaction.
    let mut out = String::new();
    for part in s.split_whitespace() {
        if part.to_lowercase().starts_with("password=") {
            out.push_str("password=*** ");
        } else {
            out.push_str(part);
            out.push(' ');
        }
    }
    out.trim_end().to_string()
}

#[derive(Debug, Deserialize)]
struct RunRequest {
    iso3: Option<String>,
    year: Option<i32>,
    seed_infections: Option<f64>,
    t_end_days: Option<f64>,
    dt_days: Option<f64>,
}

#[derive(Debug, Deserialize)]
struct LatestQuery {
    iso3: Option<String>,
    year: Option<i32>,
}

#[derive(Debug, Serialize)]
struct RunResponse {
    return_code: i32,
    run_id: String,
    iso3: String,
    year: i32,
    start_population: f64,
    end_population: f64,
    time: f64,
    seed: f64,
    population_timeline_key: String,
    hiv_infections_timeline_key: String,
}
#[tokio::main]
async fn main() {
    let pg_conn_str = std::env::var("PG_CONN_STR")
        .unwrap_or_else(|_| "host=postgres port=5432 user=airflow password=airflow dbname=viss".to_string());

    let host = std::env::var("HOST").unwrap_or_else(|_| "0.0.0.0".to_string());
    let port: u16 = std::env::var("PORT")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(8000);

    let state = AppState { pg_conn_str };

    let app = Router::new()
        .route("/healthz", get(healthz))
        .route("/run_simulation", post(run_simulation))
        .route("/population_timeline/latest", get(population_latest))
        .route("/hiv_infections_timeline/latest", get(hiv_latest))
        .route("/population_timeline/:key", get(population_by_key))
        .route("/hiv_infections_timeline/:key", get(hiv_by_key))
        .with_state(state);

    let addr: SocketAddr = format!("{}:{}", host, port).parse().expect("invalid HOST/PORT");
    println!("[vrust-api] listening on http://{}", addr);

    let listener = tokio::net::TcpListener::bind(addr).await.expect("bind failed");
    axum::serve(listener, app).await.expect("server failed");
}

async fn healthz() -> impl IntoResponse {
    Json(json!({"ok": true}))
}

async fn run_simulation(State(st): State<AppState>, Json(req): Json<RunRequest>) -> impl IntoResponse {
    // Postgres + simulation are CPU/blocking work; run on blocking pool to avoid panics.
    let pg_conn_str = st.pg_conn_str.clone();
    let join = tokio::task::spawn_blocking(move || run_simulation_sync(&pg_conn_str, req));

    match join.await {
        Ok(Ok(resp)) => (StatusCode::OK, Json(resp)).into_response(),
        Ok(Err((code, body))) => (code, Json(body)).into_response(),
        Err(e) => (
            StatusCode::INTERNAL_SERVER_ERROR,
            Json(json!({"return_code": 2, "error": format!("join error: {e}")})),
        )
            .into_response(),
    }
}

fn run_simulation_sync(pg_conn_str: &str, req: RunRequest) -> Result<RunResponse, (StatusCode, serde_json::Value)> {
    let iso3 = req.iso3.unwrap_or_else(|| "SUR".to_string()).to_uppercase();
    let year = req.year.unwrap_or(2025);

    let seed_infections = req.seed_infections.unwrap_or(10.0).max(0.0);
    let t_end = req.t_end_days.unwrap_or(365.0).max(1.0);
    let dt = req.dt_days.unwrap_or(0.25).max(1e-6);

    let run_id = format!("{}-{}-{}", iso3, year, chronoish_now_millis());

    // Build model input
    let (_labels, pop) = load_age_pyramid_5yr_pg(pg_conn_str, &iso3, year).map_err(|e| {
        (
            StatusCode::BAD_REQUEST,
            json!({"return_code": 1, "error": format!("failed to load age pyramid: {e}")}),
        )
    })?;

    let n_age = pop.len();
    let contact = synthetic_contact_matrix(n_age);

    let sigma = 1.0 / 14.0;
    let gamma = 1.0 / 180.0;
    // Mortality placeholders (per day). These should be replaced by real demography + HIV natural history.
    let mu = 0.008 / 365.0; // ~0.8% annual baseline mortality
    let mu_i_extra = 0.02 / 365.0; // additional ~2% annual mortality while infected
    let r0 = 1.5;
    let beta0 = beta0_from_r0(&contact, gamma, r0);

    let cfg = SeirsConfig {
        n_age,
        k_e: 1,
        k_i: 1,
        sigma,
        gamma,
        omega: 0.0,
        mu,
        mu_i_extra,
        beta0,
        beta_schedule: vec![(0.0, 1.0)],
        contact,
        pop: pop.clone(),
        vacc_rate: None,
    };

    let model = SeirsModel::new(cfg).map_err(|e| {
        (
            StatusCode::BAD_REQUEST,
            json!({"return_code": 1, "error": format!("invalid model config: {e}")}),
        )
    })?;

    // Seed infections proportional to population
    let total_pop: f64 = pop.iter().sum();
    let mut seeding = vec![0.0; n_age];
    if total_pop > 0.0 {
        for (i, p) in pop.iter().enumerate() {
            seeding[i] = seed_infections * (*p / total_pop);
        }
    }

    let mut state = SeirsState::init_from_seeding(&model.cfg, &seeding);
    let traj = model.simulate(&mut state, 0.0, t_end, dt);

    // Convert to timeline arrays: [[t, value], ...]
    let mut population_timeline: Vec<(f64, f64)> = Vec::with_capacity(traj.len());
    let mut infected_timeline: Vec<(f64, f64)> = Vec::with_capacity(traj.len());
    for (t, y) in &traj {
        let (s_tot, e_tot, i_tot, r_tot) = totals(&model.cfg, y);
        let pop_tot = (s_tot + e_tot + i_tot + r_tot).ceil();
        let infected_tot = i_tot.ceil();
        population_timeline.push((*t, pop_tot));
        infected_timeline.push((*t, infected_tot));
    }

    // Persist to Postgres
    persist_run(
        pg_conn_str,
        &run_id,
        &iso3,
        year,
        seed_infections,
        t_end,
        dt,
        &population_timeline,
        &infected_timeline,
    )
        .map_err(|e| {
            (
                StatusCode::INTERNAL_SERVER_ERROR,
                json!({"return_code": 2, "error": format!("failed to persist run: {e:#}")}),
            )
        })?;

    let start_population = population_timeline.first().map(|(_, v)| *v).unwrap_or(0.0);
    let end_population = population_timeline.last().map(|(_, v)| *v).unwrap_or(start_population);

    Ok(RunResponse {
        return_code: 0,
        run_id: run_id.clone(),
        iso3: iso3.clone(),
        year,
        start_population,
        end_population,
        time: t_end,
        seed: seed_infections,
        population_timeline_key: format!("seirs:{}:population", run_id),
        hiv_infections_timeline_key: format!("seirs:{}:infected", run_id),
    })
}

async fn population_latest(State(st): State<AppState>, Query(q): Query<LatestQuery>) -> impl IntoResponse {
    let pg_conn_str = st.pg_conn_str.clone();
    let iso3 = q.iso3.unwrap_or_else(|| "SUR".to_string());
    let year = q.year.unwrap_or(2025);
    let join = tokio::task::spawn_blocking(move || fetch_latest_series(&pg_conn_str, &iso3, year, "population"));
    match join.await {
        Ok(Ok(v)) => (StatusCode::OK, Json(v)).into_response(),
        Ok(Err(e)) => (StatusCode::NOT_FOUND, Json(json!({"error": e}))).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, Json(json!({"error": format!("join error: {e}")}))).into_response(),
    }
}

async fn hiv_latest(State(st): State<AppState>, Query(q): Query<LatestQuery>) -> impl IntoResponse {
    let pg_conn_str = st.pg_conn_str.clone();
    let iso3 = q.iso3.unwrap_or_else(|| "SUR".to_string());
    let year = q.year.unwrap_or(2025);
    let join = tokio::task::spawn_blocking(move || fetch_latest_series(&pg_conn_str, &iso3, year, "infected"));
    match join.await {
        Ok(Ok(v)) => (StatusCode::OK, Json(v)).into_response(),
        Ok(Err(e)) => (StatusCode::NOT_FOUND, Json(json!({"error": e}))).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, Json(json!({"error": format!("join error: {e}")}))).into_response(),
    }
}

async fn population_by_key(State(st): State<AppState>, Path(key): Path<String>) -> impl IntoResponse {
    let pg_conn_str = st.pg_conn_str.clone();
    let join = tokio::task::spawn_blocking(move || fetch_series_by_key(&pg_conn_str, &key, "population"));
    match join.await {
        Ok(Ok(v)) => (StatusCode::OK, Json(v)).into_response(),
        Ok(Err(e)) => (StatusCode::NOT_FOUND, Json(json!({"error": e}))).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, Json(json!({"error": format!("join error: {e}")}))).into_response(),
    }
}

async fn hiv_by_key(State(st): State<AppState>, Path(key): Path<String>) -> impl IntoResponse {
    let pg_conn_str = st.pg_conn_str.clone();
    let join = tokio::task::spawn_blocking(move || fetch_series_by_key(&pg_conn_str, &key, "infected"));
    match join.await {
        Ok(Ok(v)) => (StatusCode::OK, Json(v)).into_response(),
        Ok(Err(e)) => (StatusCode::NOT_FOUND, Json(json!({"error": e}))).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, Json(json!({"error": format!("join error: {e}")}))).into_response(),
    }
}

fn persist_run(
    pg_conn_str: &str,
    run_id: &str,
    iso3: &str,
    year: i32,
    seed_infections: f64,
    t_end: f64,
    dt: f64,
    population: &[(f64, f64)],
    infected: &[(f64, f64)],
) -> anyhow::Result<()> {
    let mut client = Client::connect(pg_conn_str, NoTls)
        .with_context(|| format!("postgres connect failed (conn_str={})", redact_conn_str(pg_conn_str)))?;

    client
        .batch_execute(
            r#"
            CREATE TABLE IF NOT EXISTS seirs_runs (
              id TEXT PRIMARY KEY,
              iso3 TEXT NOT NULL,
              year INTEGER NOT NULL,
              params JSONB,
              created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
            );

            CREATE INDEX IF NOT EXISTS seirs_runs_iso3_year_created_at_idx
              ON seirs_runs (iso3, year, created_at DESC);

            CREATE TABLE IF NOT EXISTS seirs_series_points (
              run_id TEXT NOT NULL REFERENCES seirs_runs(id) ON DELETE CASCADE,
              t DOUBLE PRECISION NOT NULL,
              population DOUBLE PRECISION NOT NULL,
              infected DOUBLE PRECISION NOT NULL,
              PRIMARY KEY (run_id, t)
            );

            CREATE INDEX IF NOT EXISTS seirs_series_points_run_idx
              ON seirs_series_points (run_id);
            "#,
        )
    .context("postgres schema ensure failed")?;

    let _ = (seed_infections, t_end, dt);
    client.execute(
        "INSERT INTO seirs_runs (id, iso3, year, params) VALUES ($1,$2,$3,NULL) ON CONFLICT (id) DO NOTHING",
        &[&run_id, &iso3, &year],
    )
    .context("insert into seirs_runs failed")?;

    // overwrite any existing points for this run_id
    client
        .execute("DELETE FROM seirs_series_points WHERE run_id = $1", &[&run_id])
        .context("delete existing seirs_series_points failed")?;

    let stmt = client
        .prepare("INSERT INTO seirs_series_points (run_id, t, population, infected) VALUES ($1,$2,$3,$4)")
        .context("prepare insert seirs_series_points failed")?;

    for ((t1, p), (t2, i)) in population.iter().zip(infected.iter()) {
        anyhow::ensure!((t1 - t2).abs() < 1e-9, "timeline t mismatch");
        client
            .execute(&stmt, &[&run_id, t1, p, i])
            .with_context(|| format!("insert seirs_series_points failed at t={}", t1))?;
    }

    Ok(())
}

fn fetch_latest_series(pg_conn_str: &str, iso3: &str, year: i32, kind: &str) -> Result<Vec<[f64; 2]>, String> {
    let mut client = Client::connect(pg_conn_str, NoTls).map_err(|e| e.to_string())?;

    let row = client
        .query_opt(
            "SELECT id FROM seirs_runs WHERE iso3=$1 AND year=$2 ORDER BY created_at DESC LIMIT 1",
            &[&iso3.to_uppercase(), &year],
        )
        .map_err(|e| e.to_string())?;

    let run_id: String = match row {
        Some(r) => r.get(0),
        None => return Err("no run found".to_string()),
    };

    fetch_series_for_run(&mut client, &run_id, kind)
}

fn fetch_series_by_key(pg_conn_str: &str, key: &str, kind: &str) -> Result<Vec<[f64; 2]>, String> {
    // expected key: seirs:<run_id>:population|infected
    let parts: Vec<&str> = key.split(':').collect();
    if parts.len() < 3 || parts[0] != "seirs" {
        return Err("invalid key".to_string());
    }
    let run_id = parts[1];
    let mut client = Client::connect(pg_conn_str, NoTls).map_err(|e| e.to_string())?;
    fetch_series_for_run(&mut client, run_id, kind)
}

fn fetch_series_for_run(client: &mut Client, run_id: &str, kind: &str) -> Result<Vec<[f64; 2]>, String> {
    let rows = client
        .query(
            "SELECT t, population, infected FROM seirs_series_points WHERE run_id=$1 ORDER BY t ASC",
            &[&run_id],
        )
        .map_err(|e| e.to_string())?;

    if rows.is_empty() {
        return Err("no points found".to_string());
    }

    let mut out: Vec<[f64; 2]> = Vec::with_capacity(rows.len());
    for r in rows {
        let t: f64 = r.get(0);
        let population: f64 = r.get(1);
        let infected: f64 = r.get(2);
        let v = match kind {
            "population" => population,
            "infected" => infected,
            _ => return Err("invalid kind".to_string()),
        };
        out.push([t, v]);
    }
    Ok(out)
}

fn totals(cfg: &SeirsConfig, y: &[f64]) -> (f64, f64, f64, f64) {
    let mut s_tot = 0.0;
    let mut e_tot = 0.0;
    let mut i_tot = 0.0;
    let mut r_tot = 0.0;
    for a in 0..cfg.n_age {
        let block = 1 + cfg.k_e + cfg.k_i + 1;
        let base = a * block;
        let s_idx = base;
        let e0 = base + 1;
        let i0 = e0 + cfg.k_e;
        let r_idx = i0 + cfg.k_i;

        s_tot += y[s_idx];
        for j in 0..cfg.k_e {
            e_tot += y[e0 + j];
        }
        for j in 0..cfg.k_i {
            i_tot += y[i0 + j];
        }
        r_tot += y[r_idx];
    }
    (s_tot, e_tot, i_tot, r_tot)
}

fn chronoish_now_millis() -> u128 {
    // avoid adding a chrono dependency just for an id
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_millis()
}
