use std::time::Instant;

use vrust::calibration::beta0_from_r0;
use vrust::model::seirs::{SeirsConfig, SeirsModel, SeirsState};

fn main() -> anyhow::Result<()> {
    // Larger toy to amplify differences
    let n_age = 16;
    // Simple contact: more within-age contact plus some mixing
    let mut contact = vec![vec![0.5; n_age]; n_age];
    for a in 0..n_age { contact[a][a] = 8.0; }
    for a in 0..n_age { for b in 0..n_age { if a != b { contact[a][b] *= 0.2; } } }

    // Synthetic population: decreasing with age
    let mut pop = vec![0.0; n_age];
    for a in 0..n_age { pop[a] = 1.0e6 * (1.0 - (a as f64)/(n_age as f64) * 0.5); }

    let r0 = 2.2;
    let gamma = 1.0 / 5.0;
    let sigma = 1.0 / 3.0;

    let beta0 = beta0_from_r0(&contact, gamma, r0);

    let cfg = SeirsConfig {
        n_age,
        k_e: 2,
        k_i: 2,
        sigma,
        gamma,
        omega: 0.0,
        beta0,
        beta_schedule: vec![(0.0, 1.0)],
        contact,
        pop: pop.clone(),
        vacc_rate: None,
    };

    // Baseline model (allocating deriv)
    let model_baseline = SeirsModel::new(cfg.clone())?;
    let mut state1 = SeirsState::init_from_seeding(&model_baseline.cfg, &vec![5.0; n_age]);

    // Optimized model (scratch + RK4 workspace)
    let mut model_opt = SeirsModel::new(cfg)?;
    let mut state2 = SeirsState::init_from_seeding(&model_opt.cfg, &vec![5.0; n_age]);

    let t0 = 0.0;
    let t_end = 365.0;
    let dt = 0.25;

    let t_start = Instant::now();
    let _traj1 = model_baseline.simulate(&mut state1, t0, t_end, dt);
    let dur1 = t_start.elapsed();

    let t_start2 = Instant::now();
    let _traj2 = model_opt.simulate_optimized(&mut state2, t0, t_end, dt);
    let dur2 = t_start2.elapsed();

    println!("baseline_ms,optimized_ms,speedup_x");
    let b_ms = dur1.as_secs_f64() * 1000.0;
    let o_ms = dur2.as_secs_f64() * 1000.0;
    println!("{:.3},{:.3},{:.2}", b_ms, o_ms, b_ms.max(1e-9)/o_ms.max(1e-9));

    Ok(())
}
