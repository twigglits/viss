use vrust::calibration::beta0_from_r0;
use vrust::io::age_pyramid_pg::load_age_pyramid_5yr_pg;
use vrust::io::contact_synth::synthetic_contact_matrix;
use vrust::model::seirs::{SeirsConfig, SeirsModel, SeirsState};

fn main() -> anyhow::Result<()> {
    // Example: connect to the same Postgres used by docker-compose (postgres-dev)
    // You can override with PG_CONN_STR env var.
    let pg_conn_str = std::env::var("PG_CONN_STR").unwrap_or_else(|_| {
        "host=127.0.0.1 port=5432 user=airflow password=airflow dbname=viss".to_string()
    });

    let iso3 = std::env::var("ISO3").unwrap_or_else(|_| "SUR".to_string());
    let year: i32 = std::env::var("YEAR").ok().and_then(|v| v.parse().ok()).unwrap_or(2025);

    let (_labels, pop) = load_age_pyramid_5yr_pg(&pg_conn_str, &iso3, year)?;
    let n_age = pop.len();

    // Temporary synthetic contact matrix (replace later with real contacts)
    let contact = synthetic_contact_matrix(n_age);

    // Disease parameters (placeholder; HIV will require different natural history later)
    let sigma = 1.0 / 14.0; // 14-day exposed period (placeholder)
    let gamma = 1.0 / 180.0; // 180-day mean infectious period (placeholder)

    // Choose a baseline R0 for calibration (placeholder)
    let r0 = 1.5;
    let beta0 = beta0_from_r0(&contact, gamma, r0);

    let cfg = SeirsConfig {
        n_age,
        k_e: 1,
        k_i: 1,
        sigma,
        gamma,
        omega: 0.0,
        beta0,
        beta_schedule: vec![(0.0, 1.0)],
        contact,
        pop: pop.clone(),
        vacc_rate: None,
    };

    let model = SeirsModel::new(cfg)?;

    // Seed 10 infections total, distributed proportional to population
    let total_pop: f64 = pop.iter().sum();
    let mut seeding = vec![0.0; n_age];
    if total_pop > 0.0 {
        for (i, p) in pop.iter().enumerate() {
            seeding[i] = 10.0 * (*p / total_pop);
        }
    }

    let mut state = SeirsState::init_from_seeding(&model.cfg, &seeding);

    let t0 = 0.0;
    let t_end = 365.0;
    let dt = 0.25;

    let traj = model.simulate(&mut state, t0, t_end, dt);

    println!("day,total_S,total_E,total_I,total_R");
    for (idx, (t, y)) in traj.iter().enumerate() {
        if idx % 4 != 0 {
            continue;
        }
        let (s_tot, e_tot, i_tot, r_tot) = totals(&model.cfg, y);
        println!("{:.0},{:.0},{:.0},{:.0},{:.0}", t, s_tot, e_tot, i_tot, r_tot);
    }

    Ok(())
}

fn totals(cfg: &SeirsConfig, y: &[f64]) -> (f64, f64, f64, f64) {
    let mut s_tot = 0.0;
    let mut e_tot = 0.0;
    let mut i_tot = 0.0;
    let mut r_tot = 0.0;
    for a in 0..cfg.n_age {
        let (s_idx, e0, i0, r_idx) = indices(cfg, a);
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

fn indices(cfg: &SeirsConfig, a: usize) -> (usize, usize, usize, usize) {
    let block = 1 + cfg.k_e + cfg.k_i + 1;
    let base = a * block;
    (base, base + 1, base + 1 + cfg.k_e, base + 1 + cfg.k_e + cfg.k_i)
}
