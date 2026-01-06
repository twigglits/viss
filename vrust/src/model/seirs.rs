use serde::{Deserialize, Serialize};

use crate::math::ode::rk4_step;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SeirsConfig {
    pub n_age: usize,
    pub k_e: usize,
    pub k_i: usize,

    // Rates (per day)
    pub sigma: f64,   // 1/incubation mean
    pub gamma: f64,   // 1/infectious mean
    pub omega: f64,   // waning from R->S (0 for no waning)

    // Mortality (per day)
    // Baseline mortality applied to all compartments.
    pub mu: f64,
    // Excess mortality applied to infected (I) compartments.
    pub mu_i_extra: f64,

    // Transmission
    pub beta0: f64,   // baseline beta
    pub beta_schedule: Vec<(f64, f64)>, // sorted by time: (t, multiplier m(t))

    // Contact matrix and population by age
    pub contact: Vec<Vec<f64>>, // C[a][b]
    pub pop: Vec<f64>,          // N[a]

    // Optional vaccination rate per age (per day), applied to S
    pub vacc_rate: Option<Vec<f64>>,
}

impl SeirsConfig {
    pub fn beta_at(&self, t: f64) -> f64 {
        if self.beta_schedule.is_empty() { return self.beta0; }
        let mut current = self.beta0;
        for (tt, m) in &self.beta_schedule {
            if t >= *tt { current = self.beta0 * *m; } else { break; }
        }
        current
    }

    pub fn check(&self) -> anyhow::Result<()> {
        anyhow::ensure!(self.contact.len() == self.n_age, "contact rows != n_age");
        anyhow::ensure!(self.contact.iter().all(|r| r.len() == self.n_age), "contact must be square n_age x n_age");
        anyhow::ensure!(self.pop.len() == self.n_age, "pop.len != n_age");
        if let Some(v) = &self.vacc_rate { anyhow::ensure!(v.len() == self.n_age, "vacc_rate.len != n_age"); }
        anyhow::ensure!(self.k_e >= 1 && self.k_i >= 1, "k_e and k_i must be >= 1");
        anyhow::ensure!(self.mu >= 0.0 && self.mu_i_extra >= 0.0, "mu and mu_i_extra must be >= 0");
        Ok(())
    }

    pub fn state_size(&self) -> usize {
        // per age: S (1) + E stages (k_e) + I stages (k_i) + R (1)
        self.n_age * (1 + self.k_e + self.k_i + 1)
    }
}

#[derive(Debug, Clone)]
pub struct SeirsState {
    pub y: Vec<f64>,
}

impl SeirsState {
    pub fn new_zero(cfg: &SeirsConfig) -> Self {
        Self { y: vec![0.0; cfg.state_size()] }
    }

    pub fn init_from_seeding(cfg: &SeirsConfig, seeding_per_age: &[f64]) -> Self {
        let mut s = Self::new_zero(cfg);
        for a in 0..cfg.n_age {
            let n = cfg.pop[a];
            let seed = seeding_per_age[a].min(n.max(0.0));
            let (idx_s, _e0, i0, _r) = indices(cfg, a);
            s.y[idx_s] = n - seed;
            s.y[i0] = seed; // place initial infections in I1
        }
        s
    }
}

fn indices(cfg: &SeirsConfig, a: usize) -> (usize, usize, usize, usize) {
    // Layout per age block:
    // S | E1..Ek | I1..Ik | R
    let block = 1 + cfg.k_e + cfg.k_i + 1;
    let base = a * block;
    let idx_s = base;
    let e0 = idx_s + 1;
    let i0 = e0 + cfg.k_e;
    let r = i0 + cfg.k_i;
    (idx_s, e0, i0, r)
}

pub struct SeirsModel {
    pub cfg: SeirsConfig,
}

impl SeirsModel {
    pub fn new(cfg: SeirsConfig) -> anyhow::Result<Self> {
        cfg.check()?;
        Ok(Self { cfg })
    }

    pub fn deriv(&self, t: f64, y: &[f64], dy: &mut [f64]) {
        dy.fill(0.0);
        let cfg = &self.cfg;

        // Compute I_a and N_a at time t (local allocations for baseline path)
        let mut i_by_age = vec![0.0; cfg.n_age];
        let mut n_by_age = vec![0.0; cfg.n_age];
        for a in 0..cfg.n_age {
            let (s_idx, e0, i0, r_idx) = indices(cfg, a);
            let s = y[s_idx];
            let r = y[r_idx];
            let mut e_sum = 0.0;
            let mut i_sum = 0.0;
            for j in 0..cfg.k_e { e_sum += y[e0 + j]; }
            for j in 0..cfg.k_i { i_sum += y[i0 + j]; }
            i_by_age[a] = i_sum;
            n_by_age[a] = s + e_sum + i_sum + r;
        }

        // Force of infection per age
        let beta = self.cfg.beta_at(t);
        let mut lambda = vec![0.0; cfg.n_age];
        for a in 0..cfg.n_age {
            let mut sum = 0.0;
            for b in 0..cfg.n_age {
                let nb = n_by_age[b];
                if nb > 0.0 { sum += cfg.contact[a][b] * i_by_age[b] / nb; }
            }
            lambda[a] = beta * sum;
        }

        // Vaccination rates per age
        let zero;
        let vacc: &[f64] = if let Some(v) = &cfg.vacc_rate { v } else { zero = vec![0.0; cfg.n_age]; &zero };

        // Transitions
        let ke_sigma = (cfg.k_e as f64) * cfg.sigma;
        let ki_gamma = (cfg.k_i as f64) * cfg.gamma;

        for a in 0..cfg.n_age {
            let (s_idx, e0, i0, r_idx) = indices(cfg, a);

            // S
            let s = y[s_idx];
            let to_e = lambda[a] * s;
            let to_r_vacc = vacc[a] * s;
            dy[s_idx] -= to_e + to_r_vacc;
            dy[s_idx] -= cfg.mu * s;

            // E stages
            dy[e0] += to_e - ke_sigma * y[e0];
            dy[e0] -= cfg.mu * y[e0];
            for j in 1..cfg.k_e {
                dy[e0 + j] += ke_sigma * y[e0 + j - 1] - ke_sigma * y[e0 + j];
                dy[e0 + j] -= cfg.mu * y[e0 + j];
            }

            // I stages
            dy[i0] += ke_sigma * y[e0 + cfg.k_e - 1] - ki_gamma * y[i0];
            dy[i0] -= (cfg.mu + cfg.mu_i_extra) * y[i0];
            for j in 1..cfg.k_i {
                dy[i0 + j] += ki_gamma * y[i0 + j - 1] - ki_gamma * y[i0 + j];
                dy[i0 + j] -= (cfg.mu + cfg.mu_i_extra) * y[i0 + j];
            }

            // R
            let inflow_r = ki_gamma * y[i0 + cfg.k_i - 1] + to_r_vacc;
            dy[r_idx] += inflow_r - cfg.omega * y[r_idx];
            dy[r_idx] -= cfg.mu * y[r_idx];
            dy[s_idx] += cfg.omega * y[r_idx];
        }
    }

    pub fn simulate(&self, state: &mut SeirsState, t0: f64, t_end: f64, dt: f64) -> Vec<(f64, Vec<f64>)> {
        let mut t = t0;
        let mut out = Vec::new();
        out.push((t, state.y.clone()));
        while t < t_end - 1e-12 {
            rk4_step(&mut state.y, t, dt, |tt, y, dy| self.deriv(tt, y, dy));
            t += dt;
            out.push((t, state.y.clone()));
        }
        out
    }
}
