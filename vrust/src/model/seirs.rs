use serde::{Deserialize, Serialize};

use crate::math::ode::{rk4_step, rk4_step_ws, Rk4Workspace};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SeirsConfig {
    pub n_age: usize,
    pub k_e: usize,
    pub k_i: usize,

    // Rates (per day)
    pub sigma: f64,   // 1/incubation mean
    pub gamma: f64,   // 1/infectious mean
    pub omega: f64,   // waning from R->S (0 for no waning)

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
    // Scratch buffers to avoid per-step allocations
    i_by_age: Vec<f64>,
    n_by_age: Vec<f64>,
    lambda: Vec<f64>,
    vacc_buf: Vec<f64>,
    rk_ws: Rk4Workspace,
}

impl SeirsModel {
    pub fn new(cfg: SeirsConfig) -> anyhow::Result<Self> {
        cfg.check()?;
        let n_age = cfg.n_age;
        Ok(Self {
            cfg,
            i_by_age: vec![0.0; n_age],
            n_by_age: vec![0.0; n_age],
            lambda: vec![0.0; n_age],
            vacc_buf: vec![0.0; n_age],
            rk_ws: Rk4Workspace::new(0),
        })
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

            // E stages
            dy[e0] += to_e - ke_sigma * y[e0];
            for j in 1..cfg.k_e {
                dy[e0 + j] += ke_sigma * y[e0 + j - 1] - ke_sigma * y[e0 + j];
            }

            // I stages
            dy[i0] += ke_sigma * y[e0 + cfg.k_e - 1] - ki_gamma * y[i0];
            for j in 1..cfg.k_i {
                dy[i0 + j] += ki_gamma * y[i0 + j - 1] - ki_gamma * y[i0 + j];
            }

            // R
            let inflow_r = ki_gamma * y[i0 + cfg.k_i - 1] + to_r_vacc;
            dy[r_idx] += inflow_r - cfg.omega * y[r_idx];
            dy[s_idx] += cfg.omega * y[r_idx];
        }
    }

    fn deriv_allocfree_mut(&mut self, t: f64, y: &[f64], dy: &mut [f64]) {
        dy.fill(0.0);
        let cfg = &self.cfg;

        // Compute I_a and N_a at time t (scratch buffers)
        for a in 0..cfg.n_age {
            let (s_idx, e0, i0, r_idx) = indices(cfg, a);
            let s = y[s_idx];
            let r = y[r_idx];
            let mut e_sum = 0.0;
            let mut i_sum = 0.0;
            for j in 0..cfg.k_e { e_sum += y[e0 + j]; }
            for j in 0..cfg.k_i { i_sum += y[i0 + j]; }
            self.i_by_age[a] = i_sum;
            self.n_by_age[a] = s + e_sum + i_sum + r;
        }

        // Force of infection per age
        let beta = self.cfg.beta_at(t);
        for a in 0..cfg.n_age {
            let mut sum = 0.0;
            for b in 0..cfg.n_age {
                let nb = self.n_by_age[b];
                if nb > 0.0 { sum += cfg.contact[a][b] * self.i_by_age[b] / nb; }
            }
            self.lambda[a] = beta * sum;
        }

        // Vaccination rates per age into vacc_buf
        if let Some(v) = &cfg.vacc_rate {
            self.vacc_buf.copy_from_slice(v);
        } else {
            for a in 0..cfg.n_age { self.vacc_buf[a] = 0.0; }
        }

        // Transitions
        let ke_sigma = (cfg.k_e as f64) * cfg.sigma;
        let ki_gamma = (cfg.k_i as f64) * cfg.gamma;

        for a in 0..cfg.n_age {
            let (s_idx, e0, i0, r_idx) = indices(cfg, a);

            // S
            let s = y[s_idx];
            let to_e = self.lambda[a] * s;
            let to_r_vacc = self.vacc_buf[a] * s;
            dy[s_idx] -= to_e + to_r_vacc;

            // E stages
            dy[e0] += to_e - ke_sigma * y[e0];
            for j in 1..cfg.k_e {
                dy[e0 + j] += ke_sigma * y[e0 + j - 1] - ke_sigma * y[e0 + j];
            }

            // I stages
            dy[i0] += ke_sigma * y[e0 + cfg.k_e - 1] - ki_gamma * y[i0];
            for j in 1..cfg.k_i {
                dy[i0 + j] += ki_gamma * y[i0 + j - 1] - ki_gamma * y[i0 + j];
            }

            // R
            let inflow_r = ki_gamma * y[i0 + cfg.k_i - 1] + to_r_vacc;
            dy[r_idx] += inflow_r - cfg.omega * y[r_idx];
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

    pub fn simulate_optimized(&mut self, state: &mut SeirsState, t0: f64, t_end: f64, dt: f64) -> Vec<(f64, Vec<f64>)> {
        let mut t = t0;
        let mut out = Vec::new();
        out.push((t, state.y.clone()));
        let n = state.y.len();
        self.rk_ws.resize(n);
        let mut ytmp = vec![0.0; n];

        // Take a snapshot of cfg to avoid repeated immutable borrows of self
        let cfg = self.cfg.clone();
        let na = cfg.n_age;
        let ke_sigma = (cfg.k_e as f64) * cfg.sigma;
        let ki_gamma = (cfg.k_i as f64) * cfg.gamma;
        let vacc_const: Vec<f64> = if let Some(v) = &cfg.vacc_rate { v.clone() } else { vec![0.0; na] };

        // Local scratch (single allocation per simulate)
        let mut i_by_age = vec![0.0; na];
        let mut n_by_age = vec![0.0; na];
        let mut lambda = vec![0.0; na];

        let mut compute_deriv = |tt: f64, y: &[f64], dy: &mut [f64]| {
            dy.fill(0.0);
            // sums per age
            for a in 0..na {
                let (s_idx, e0, i0, r_idx) = indices(&cfg, a);
                let s = y[s_idx];
                let r = y[r_idx];
                let mut e_sum = 0.0;
                let mut i_sum = 0.0;
                for j in 0..cfg.k_e { e_sum += y[e0 + j]; }
                for j in 0..cfg.k_i { i_sum += y[i0 + j]; }
                i_by_age[a] = i_sum;
                n_by_age[a] = s + e_sum + i_sum + r;
            }
            // Force of infection
            let beta = cfg.beta_at(tt);
            for a in 0..na {
                let mut sum = 0.0;
                for b in 0..na {
                    let nb = n_by_age[b];
                    if nb > 0.0 { sum += cfg.contact[a][b] * i_by_age[b] / nb; }
                }
                lambda[a] = beta * sum;
            }
            // Transitions
            for a in 0..na {
                let (s_idx, e0, i0, r_idx) = indices(&cfg, a);
                let s = y[s_idx];
                let to_e = lambda[a] * s;
                let to_r_vacc = vacc_const[a] * s;
                dy[s_idx] -= to_e + to_r_vacc;
                dy[e0] += to_e - ke_sigma * y[e0];
                for j in 1..cfg.k_e { dy[e0 + j] += ke_sigma * y[e0 + j - 1] - ke_sigma * y[e0 + j]; }
                dy[i0] += ke_sigma * y[e0 + cfg.k_e - 1] - ki_gamma * y[i0];
                for j in 1..cfg.k_i { dy[i0 + j] += ki_gamma * y[i0 + j - 1] - ki_gamma * y[i0 + j]; }
                let inflow_r = ki_gamma * y[i0 + cfg.k_i - 1] + to_r_vacc;
                dy[r_idx] += inflow_r - cfg.omega * y[r_idx];
                dy[s_idx] += cfg.omega * y[r_idx];
            }
        };
        while t < t_end - 1e-12 {
            // k1
            compute_deriv(t, &state.y, &mut self.rk_ws.k1);

            // ytmp = y + 0.5*dt*k1
            for i in 0..n { ytmp[i] = state.y[i] + 0.5 * dt * self.rk_ws.k1[i]; }
            // k2
            compute_deriv(t + 0.5 * dt, &ytmp, &mut self.rk_ws.k2);

            // ytmp = y + 0.5*dt*k2
            for i in 0..n { ytmp[i] = state.y[i] + 0.5 * dt * self.rk_ws.k2[i]; }
            // k3
            compute_deriv(t + 0.5 * dt, &ytmp, &mut self.rk_ws.k3);

            // ytmp = y + dt*k3
            for i in 0..n { ytmp[i] = state.y[i] + dt * self.rk_ws.k3[i]; }
            // k4
            compute_deriv(t + dt, &ytmp, &mut self.rk_ws.k4);

            // y += (dt/6)*(k1 + 2k2 + 2k3 + k4)
            for i in 0..n {
                state.y[i] += (dt / 6.0)
                    * (self.rk_ws.k1[i]
                        + 2.0 * self.rk_ws.k2[i]
                        + 2.0 * self.rk_ws.k3[i]
                        + self.rk_ws.k4[i]);
            }

            t += dt;
            out.push((t, state.y.clone()));
        }
        out
    }
}
