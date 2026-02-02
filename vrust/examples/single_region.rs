use vrust::calibration::beta0_from_r0;
use vrust::model::seirs::{SeirsConfig, SeirsModel, SeirsState};

fn main() -> anyhow::Result<()> {
    // Toy 2-age example; replace with real CSV loaders later.
    let contact = vec![
        vec![8.0, 2.0],
        vec![2.0, 6.0],
    ];
    let pop = vec![5_000_000.0, 4_000_000.0]; // I get rid of scientific notation, because it's always confusing, added underscores for readability.

    let r0 = 2.5;
    let gamma = 1.0 / 5.0; // infectious mean 5 days
    let sigma = 1.0 / 3.0; // incubation mean 3 days

    // Mortality placeholders (per day)
    let mu = 0.008 / 365.0;
    let mu_i_extra = 0.02 / 365.0;

    let beta0 = beta0_from_r0(&contact, gamma, r0);

    let cfg = SeirsConfig {
        n_age: 2,
        k_e: 2,
        k_i: 2,
        sigma,
        gamma,
        omega: 0.0,
        mu,
        mu_i_extra,
        beta0,
        beta_schedule: vec![
            (0.0, 1.0),
            (60.0, 0.8),
            (120.0, 1.1),
        ],
        contact,
        pop: pop.clone(),
        aging_rate_per_day: None,
        fertility_per_day: None,
        female_fraction: 0.5,
        vacc_rate: None,
    };

    let model = SeirsModel::new(cfg)?;

    // Seed 10 initial infections in each age group
    let mut state = SeirsState::init_from_seeding(&model.cfg, &[10.0, 10.0]);

    // Simulate for 360 days with dt=0.25
    let traj = model.simulate(&mut state, 0.0, 360.0, 0.25);

    // Print daily summary (every 4 steps)
    println!("day,total_S,total_E,total_I,total_R");
    for (idx, (t, y)) in traj.iter().enumerate() {
        if idx % 4 != 0 { continue; }
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
        for j in 0..cfg.k_e { e_tot += y[e0 + j]; }
        for j in 0..cfg.k_i { i_tot += y[i0 + j]; }
        r_tot += y[r_idx];
    }
    (s_tot, e_tot, i_tot, r_tot)
}

fn indices(cfg: &SeirsConfig, a: usize) -> (usize, usize, usize, usize) {
    let block = 1 + cfg.k_e + cfg.k_i + 1;
    let base = a * block;
    (base, base + 1, base + 1 + cfg.k_e, base + 1 + cfg.k_e + cfg.k_i)
}
