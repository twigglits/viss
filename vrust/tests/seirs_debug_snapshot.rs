use vrust::io::contact_synth::synthetic_contact_matrix;
use vrust::io::debug_log::write_seirs_debug_log;
use vrust::model::seirs::{SeirsConfig, SeirsModel, SeirsState};

fn totals(cfg: &SeirsConfig, y: &[f64]) -> (f64, f64, f64, f64) {
    let mut s = 0.0;
    let mut e = 0.0;
    let mut i = 0.0;
    let mut r = 0.0;
    for a in 0..cfg.n_age {
        let block = 1 + cfg.k_e + cfg.k_i + 1;
        let base = a * block;
        s += y[base];
        for k in 0..cfg.k_e {
            e += y[base + 1 + k];
        }
        for k in 0..cfg.k_i {
            i += y[base + 1 + cfg.k_e + k];
        }
        r += y[base + 1 + cfg.k_e + cfg.k_i];
    }
    (s, e, i, r)
}

#[test]
fn seirs_debug_log_snapshot_small() {
    let n_age = 6;
    let pop = vec![1000.0, 900.0, 800.0, 700.0, 600.0, 500.0];
    let contact = synthetic_contact_matrix(n_age);

    let cfg = SeirsConfig {
        n_age,
        k_e: 1,
        k_i: 1,
        sigma: 1.0 / 14.0,
        gamma: 1.0 / 180.0,
        omega: 0.0,
        mu: 0.0,
        mu_i_extra: 0.0,
        beta0: 0.06,
        beta_schedule: vec![(0.0, 1.0)],
        contact,
        pop: pop.clone(),
        aging_rate_per_day: None,
        fertility_per_day: None,
        female_fraction: 0.5,
        vacc_rate: None,
    };

    let model = SeirsModel::new(cfg).expect("model config invalid");

    let total_pop: f64 = pop.iter().sum();
    let seed_infections = 10.0;
    let mut seeding = vec![0.0; n_age];
    for (i, p) in pop.iter().enumerate() {
        seeding[i] = seed_infections * (*p / total_pop);
    }

    let mut state = SeirsState::init_from_seeding(&model.cfg, &seeding);
    let t_end = 60.0;
    let dt = 1.0;
    let traj = model.simulate(&mut state, 0.0, t_end, dt);

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

    let tmp = tempfile::tempdir().expect("tempdir");
    let run_id = "TEST-SMALL";
    let path = write_seirs_debug_log(
        tmp.path(),
        run_id,
        "TST",
        2000,
        seed_infections,
        t_end,
        dt,
        &population_timeline,
        &infected_timeline,
        &incidence_timeline,
    )
    .expect("write debug log");

    let s = std::fs::read_to_string(path).expect("read debug log");
    insta::assert_snapshot!(s);
}
