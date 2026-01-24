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
use vrust::io::age_pyramid_pg::{load_age_pyramid_5yr_pg, AGE_BINS_5YR};
use vrust::io::contact_synth::synthetic_contact_matrix;
use vrust::io::debug_log::write_seirs_debug_log;
use vrust::model::seirs::{SeirsConfig, SeirsModel, SeirsState};

#[derive(Clone)]
struct AppState {
    pg_conn_str: String,
}

async fn get_un_data_indicator_location_years(
    State(st): State<AppState>,
    Path(p): Path<UnDataPath>,
) -> impl IntoResponse {
    let indicator = p.indicator.trim().to_string();
    if indicator != "ASFR5" {
        return (
            StatusCode::BAD_REQUEST,
            Json(json!({"error": "only ASFR5 is supported by this cached endpoint"})),
        )
            .into_response();
    }

    let iso3 = p.location.trim().to_uppercase();
    if iso3.len() != 3 {
        return (
            StatusCode::BAD_REQUEST,
            Json(json!({"error": "location must be an ISO3 code in this cached endpoint"})),
        )
            .into_response();
    }
    let start = p.start;
    let end = p.end;
    if start > end {
        return (
            StatusCode::BAD_REQUEST,
            Json(json!({"error": "start must be <= end"})),
        )
            .into_response();
    }

    let pg_conn_str = st.pg_conn_str.clone();
    let join = tokio::task::spawn_blocking(move || {
        let mut client = Client::connect(&pg_conn_str, NoTls)
            .with_context(|| "Failed to connect to Postgres")?;

        let rows = client
            .query(
                "SELECT year, variant_short_name, age_min, age_max, asfr FROM asfr_5yr WHERE iso3 = $1 AND year >= $2 AND year <= $3 ORDER BY year ASC, variant_short_name ASC, age_min ASC",
                &[&iso3, &start, &end],
            )
            .with_context(|| "Failed to query asfr_5yr")?;

        let mut out: Vec<UnAsfrRow> = Vec::with_capacity(rows.len());
        for row in rows {
            let year: i32 = row.get(0);
            let variant_short_name: String = row.get(1);
            let age_min: i32 = row.get(2);
            let age_max: i32 = row.get(3);
            let asfr: f64 = row.get(4);
            out.push(UnAsfrRow {
                location_id: iso3.clone(),
                // We do not persist sexId in the cache currently; use 3 (both sexes) as a safe default.
                // The DAG logs when it falls back from female (2) to both sexes (3).
                sex_id: 3,
                variant_short_name: variant_short_name,
                age_start: age_min,
                age_end: age_max,
                time_label: year,
                value: asfr,
            });
        }

        Ok::<Vec<UnAsfrRow>, anyhow::Error>(out)
    });

    match join.await {
        Ok(Ok(rows)) => (StatusCode::OK, Json(rows)).into_response(),
        Ok(Err(e)) => (
            StatusCode::INTERNAL_SERVER_ERROR,
            Json(json!({"error": format!("failed to load cached ASFR from Postgres: {e}")})),
        )
            .into_response(),
        Err(e) => (
            StatusCode::INTERNAL_SERVER_ERROR,
            Json(json!({"error": format!("join error: {e}")})),
        )
            .into_response(),
    }
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
    debug: Option<bool>,
    debug_id: Option<String>,
}

#[derive(Debug, Deserialize)]
struct LatestQuery {
    iso3: Option<String>,
    year: Option<i32>,
}

#[derive(Debug, Deserialize)]
struct UnDataPath {
    indicator: String,
    location: String,
    start: i32,
    end: i32,
}

#[derive(Debug, Deserialize)]
struct AsfrQuery {
    iso3: String,
    year: i32,
}

#[derive(Debug, Serialize)]
struct AsfrBin {
    variant_short_name: String,
    age_min: i32,
    age_max: i32,
    asfr: f64,
}

#[derive(Debug, Serialize)]
struct AsfrResponse {
    iso3: String,
    year: i32,
    source: String,
    indicator: String,
    unit: String,
    ages: Vec<AsfrBin>,
}

#[derive(Debug, Serialize)]
struct UnAsfrRow {
    #[serde(rename = "locationId")]
    location_id: String,
    #[serde(rename = "sexId")]
    sex_id: i32,
    #[serde(rename = "variantShortName")]
    variant_short_name: String,
    #[serde(rename = "ageStart")]
    age_start: i32,
    #[serde(rename = "ageEnd")]
    age_end: i32,
    #[serde(rename = "timeLabel")]
    time_label: i32,
    value: f64,
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
    hiv_incidence_timeline_key: String,
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
        .route("/demography/asfr", get(get_asfr))
        .route(
            "/api/v1/data/indicators/:indicator/locations/:location/start/:start/end/:end",
            get(get_un_data_indicator_location_years),
        )
        .route("/population_timeline/latest", get(population_latest))
        .route("/hiv_infections_timeline/latest", get(hiv_latest))
        .route("/hiv_incidence_timeline/latest", get(incidence_latest))
        .route("/population_timeline/:key", get(population_by_key))
        .route("/hiv_infections_timeline/:key", get(hiv_by_key))
        .route("/hiv_incidence_timeline/:key", get(incidence_by_key))
        .with_state(state);

    let addr: SocketAddr = format!("{}:{}", host, port).parse().expect("invalid HOST/PORT");
    println!("[vrust-api] listening on http://{}", addr);

    let listener = tokio::net::TcpListener::bind(addr).await.expect("bind failed");
    axum::serve(listener, app).await.expect("server failed");
}

async fn healthz() -> impl IntoResponse {
    Json(json!({"ok": true}))
}

async fn get_asfr(State(st): State<AppState>, Query(q): Query<AsfrQuery>) -> impl IntoResponse {
    let iso3 = q.iso3.trim().to_uppercase();
    if iso3.len() != 3 {
        return (
            StatusCode::BAD_REQUEST,
            Json(json!({"error": "iso3 must be a 3-letter code"})),
        )
            .into_response();
    }
    if q.year < 1950 || q.year > 2100 {
        return (
            StatusCode::BAD_REQUEST,
            Json(json!({"error": "year out of supported range"})),
        )
            .into_response();
    }

    let pg_conn_str = st.pg_conn_str.clone();
    let join = tokio::task::spawn_blocking(move || {
        let mut client = Client::connect(&pg_conn_str, NoTls)
            .with_context(|| "Failed to connect to Postgres")?;

        // Return the WPP 5-year fertility ages (15-49), including uncertainty variants.
        let rows = client
            .query(
                "SELECT variant_short_name, age_min, age_max, asfr, source, release FROM asfr_5yr WHERE iso3 = $1 AND year = $2 ORDER BY variant_short_name ASC, age_min ASC",
                &[&iso3, &q.year],
            )
            .with_context(|| "Failed to query asfr_5yr")?;

        let mut bins: Vec<AsfrBin> = Vec::with_capacity(rows.len());
        let mut source: Option<String> = None;
        let mut release: Option<String> = None;
        for row in rows {
            let variant_short_name: String = row.get(0);
            let age_min: i32 = row.get(1);
            let age_max: i32 = row.get(2);
            let asfr: f64 = row.get(3);
            let s: String = row.get(4);
            let r: String = row.get(5);
            if source.is_none() {
                source = Some(s);
            }
            if release.is_none() {
                release = Some(r);
            }
            bins.push(AsfrBin {
                variant_short_name,
                age_min,
                age_max,
                asfr,
            });
        }

        if bins.is_empty() {
            anyhow::bail!("no rows");
        }

        Ok::<AsfrResponse, anyhow::Error>(AsfrResponse {
            iso3,
            year: q.year,
            source: format!("{} {} (cached)", source.unwrap_or_else(|| "UN_WPP".to_string()), release.unwrap_or_else(|| "".to_string())).trim().to_string(),
            indicator: "ASFR5".to_string(),
            unit: "births_per_woman_per_year".to_string(),
            ages: bins,
        })
    });

    match join.await {
        Ok(Ok(resp)) => (StatusCode::OK, Json(resp)).into_response(),
        Ok(Err(e)) => {
            let msg = e.to_string();
            if msg.contains("no rows") {
                (
                    StatusCode::NOT_FOUND,
                    Json(json!({
                        "error": "ASFR not ingested for iso3/year. Run Airflow DAG unwpp_asfr_5yr_sur_2015_2025.",
                        "iso3": q.iso3,
                        "year": q.year
                    })),
                )
                    .into_response()
            } else {
                (
                    StatusCode::INTERNAL_SERVER_ERROR,
                    Json(json!({"error": format!("failed to load ASFR from Postgres: {e}")})),
                )
                    .into_response()
            }
        }
        Err(e) => (
            StatusCode::INTERNAL_SERVER_ERROR,
            Json(json!({"error": format!("join error: {e}")})),
        )
            .into_response(),
    }
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

    let debug = req.debug.unwrap_or(false);
    let debug_id = req.debug_id.clone().unwrap_or_default();

    let run_id = if debug && !debug_id.trim().is_empty() {
        format!("{}-{}-{}", iso3, year, debug_id.trim())
    } else {
        format!("{}-{}-{}", iso3, year, chronoish_now_millis())
    };

    // Build model input
    let (_labels, pop) = load_age_pyramid_5yr_pg(pg_conn_str, &iso3, year).map_err(|e| {
        (
            StatusCode::BAD_REQUEST,
            json!({"return_code": 1, "error": format!("failed to load age pyramid: {e}")}),
        )
    })?;

    let fertility_per_day = load_asfr_fertility_per_day_pg(pg_conn_str, &iso3, year, pop.len()).map_err(|e| {
        (
            StatusCode::BAD_REQUEST,
            json!({"return_code": 1, "error": format!("failed to load ASFR fertility: {e}")}),
        )
    })?;

    let aging_rate_per_day = aging_rates_per_day_from_bins(pop.len()).map_err(|e| {
        (
            StatusCode::BAD_REQUEST,
            json!({"return_code": 1, "error": format!("failed to derive aging rates: {e}")}),
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
        aging_rate_per_day: Some(aging_rate_per_day),
        fertility_per_day: Some(fertility_per_day),
        female_fraction: 0.5,
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
    let mut incidence_timeline: Vec<(f64, f64)> = Vec::with_capacity(traj.len());
    for (t, y) in &traj {
        let (s_tot, e_tot, i_tot, r_tot) = totals(&model.cfg, y);
        let pop_tot = (s_tot + e_tot + i_tot + r_tot).ceil();
        let infected_tot = i_tot.ceil();
        population_timeline.push((*t, pop_tot));
        infected_timeline.push((*t, infected_tot));
        let denom = (s_tot + e_tot + i_tot + r_tot).max(0.0);
        let incidence_pct = if denom > 0.0 { (100.0 * i_tot / denom).max(0.0) } else { 0.0 };
        incidence_timeline.push((*t, incidence_pct));
    }

    if debug {
        let log_dir = std::env::var("VRUST_LOG_DIR").unwrap_or_else(|_| "logs".to_string());
        if let Err(e) = write_debug_log(
            &log_dir,
            &run_id,
            &iso3,
            year,
            seed_infections,
            t_end,
            dt,
            &population_timeline,
            &infected_timeline,
            &incidence_timeline,
        ) {
            eprintln!("[vrust-api] debug log write failed: {e:#}");
        }
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
        &incidence_timeline,
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
        hiv_incidence_timeline_key: format!("seirs:{}:incidence", run_id),
    })
}

fn write_debug_log(
    out_dir: &str,
    run_id: &str,
    iso3: &str,
    year: i32,
    seed_infections: f64,
    t_end: f64,
    dt: f64,
    population: &[(f64, f64)],
    infected: &[(f64, f64)],
    incidence: &[(f64, f64)],
) -> anyhow::Result<std::path::PathBuf> {
    write_seirs_debug_log(
        out_dir,
        run_id,
        iso3,
        year,
        seed_infections,
        t_end,
        dt,
        population,
        infected,
        incidence,
    )
}

fn aging_rates_per_day_from_bins(n_age: usize) -> anyhow::Result<Vec<f64>> {
    anyhow::ensure!(n_age == AGE_BINS_5YR.len(), "unexpected n_age: expected {} got {}", AGE_BINS_5YR.len(), n_age);
    let mut out = Vec::with_capacity(n_age);
    for (i, b) in AGE_BINS_5YR.iter().enumerate() {
        if i == n_age - 1 {
            out.push(0.0);
            continue;
        }
        if let Some((a, z)) = b.split_once('-') {
            let a: i32 = a.parse().with_context(|| format!("invalid age bin start: {b}"))?;
            let z: i32 = z.parse().with_context(|| format!("invalid age bin end: {b}"))?;
            let width_years = (z - a + 1) as f64;
            anyhow::ensure!(width_years > 0.0, "invalid width for bin {b}");
            out.push(1.0 / (width_years * 365.0));
        } else if b.ends_with('+') {
            out.push(0.0);
        } else {
            anyhow::bail!("unsupported age bin format: {b}");
        }
    }
    Ok(out)
}

fn load_asfr_fertility_per_day_pg(pg_conn_str: &str, iso3: &str, year: i32, n_age: usize) -> anyhow::Result<Vec<f64>> {
    anyhow::ensure!(n_age == AGE_BINS_5YR.len(), "unexpected n_age: expected {} got {}", AGE_BINS_5YR.len(), n_age);
    let mut client = Client::connect(pg_conn_str, NoTls).with_context(|| "Failed to connect to Postgres")?;

    // Use MEDIAN variant as the default fertility schedule.
    let rows = client
        .query(
            "SELECT age_min, age_max, asfr FROM asfr_5yr WHERE iso3 = $1 AND year = $2 AND variant_short_name = 'MEDIAN' ORDER BY age_min ASC",
            &[&iso3.to_uppercase(), &year],
        )
        .with_context(|| "Failed to query asfr_5yr")?;

    anyhow::ensure!(!rows.is_empty(), "no ASFR rows found for iso3={} year={} variant=MEDIAN", iso3, year);

    // Aggregate ASFR rows (which may be single-year or 5-year bins) into our 5-year model bins.
    // We do overlap-weighted averaging so both granular and coarse source bins work.
    // NOTE: UN/WPP ASFR is typically reported as births per 1,000 women per year.
    let mut fert_per_year_num = vec![0.0_f64; n_age];
    let mut fert_per_year_den = vec![0.0_f64; n_age];

    for row in rows {
        let age_min: i32 = row.get(0);
        let age_max: i32 = row.get(1);
        let asfr_per_1000_women_per_year: f64 = row.get(2);

        // Only use reproductive ages.
        if age_max < 15 || age_min > 49 {
            continue;
        }

        for (idx, b) in AGE_BINS_5YR.iter().enumerate() {
            let (bin_min, bin_max) = if let Some((a, z)) = b.split_once('-') {
                (a.parse::<i32>().with_context(|| format!("invalid age bin start: {b}"))?, z.parse::<i32>().with_context(|| format!("invalid age bin end: {b}"))?)
            } else {
                continue;
            };

            // Overlap in years (inclusive integer ages)
            let lo = age_min.max(bin_min);
            let hi = age_max.min(bin_max);
            if hi < lo {
                continue;
            }
            let overlap_years = (hi - lo + 1) as f64;
            fert_per_year_num[idx] += asfr_per_1000_women_per_year.max(0.0) * overlap_years;
            fert_per_year_den[idx] += overlap_years;
        }
    }

    let mut fert_per_day = vec![0.0_f64; n_age];
    for i in 0..n_age {
        if fert_per_year_den[i] > 0.0 {
            let avg_per_year = fert_per_year_num[i] / fert_per_year_den[i];
            // births per 1,000 women per year -> births per woman per day
            fert_per_day[i] = (avg_per_year / 1000.0 / 365.0).max(0.0);
        }
    }

    Ok(fert_per_day)
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

async fn incidence_latest(State(st): State<AppState>, Query(q): Query<LatestQuery>) -> impl IntoResponse {
    let pg_conn_str = st.pg_conn_str.clone();
    let iso3 = q.iso3.unwrap_or_else(|| "SUR".to_string());
    let year = q.year.unwrap_or(2025);
    let join = tokio::task::spawn_blocking(move || fetch_latest_series(&pg_conn_str, &iso3, year, "incidence"));
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

async fn incidence_by_key(State(st): State<AppState>, Path(key): Path<String>) -> impl IntoResponse {
    let pg_conn_str = st.pg_conn_str.clone();
    let join = tokio::task::spawn_blocking(move || fetch_series_by_key(&pg_conn_str, &key, "incidence"));
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
    incidence: &[(f64, f64)],
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
              incidence_pct DOUBLE PRECISION,
              PRIMARY KEY (run_id, t)
            );

            CREATE INDEX IF NOT EXISTS seirs_series_points_run_idx
              ON seirs_series_points (run_id);
            "#,
        )
    .context("postgres schema ensure failed")?;

    // Backfill/migrate older schema versions
    client
        .execute(
            "ALTER TABLE seirs_series_points ADD COLUMN IF NOT EXISTS incidence_pct DOUBLE PRECISION",
            &[],
        )
        .context("alter seirs_series_points add incidence_pct failed")?;

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
        .prepare("INSERT INTO seirs_series_points (run_id, t, population, infected, incidence_pct) VALUES ($1,$2,$3,$4,$5)")
        .context("prepare insert seirs_series_points failed")?;

    for (((t1, p), (t2, i)), (t3, inc)) in population.iter().zip(infected.iter()).zip(incidence.iter()) {
        anyhow::ensure!((t1 - t2).abs() < 1e-9, "timeline t mismatch");
        anyhow::ensure!((t1 - t3).abs() < 1e-9, "timeline t mismatch");
        client
            .execute(&stmt, &[&run_id, t1, p, i, inc])
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
            "SELECT t, population, infected, COALESCE(incidence_pct, CASE WHEN population > 0 THEN (infected / population) * 100.0 ELSE 0 END) AS incidence_pct FROM seirs_series_points WHERE run_id=$1 ORDER BY t ASC",
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
        let incidence_pct: f64 = r.get(3);
        let v = match kind {
            "population" => population,
            "infected" => infected,
            "incidence" => incidence_pct,
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
