use serde::{Deserialize, Serialize};

use crate::math::ode::rk4_step;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SirConfig {
    pub n_age: usize,
    pub k_i: usize,

    pub gamma: f64,

    pub mu: f64,
    pub mu_i_extra: f64,

    pub beta0: f64,
    pub beta_schedule: Vec<(f64, f64)>,

    pub contact: Vec<Vec<f64>>,
    pub pop: Vec<f64>,

    pub aging_rate_per_day: Option<Vec<f64>>,
    pub fertility_per_day: Option<Vec<f64>>,
    pub female_fraction: f64,

    pub vacc_rate: Option<Vec<f64>>,
}

impl SirConfig {
    pub fn beta_at(&self, t: f64) -> f64 {
        if self.beta_schedule.is_empty() {
            return self.beta0;
        }
        let mut current = self.beta0;
        for (tt, m) in &self.beta_schedule {
            if t >= *tt {
                current = self.beta0 * *m;
            } else {
                break;
            }
        }
        current
    }

    pub fn check(&self) -> anyhow::Result<()> {
        anyhow::ensure!(self.contact.len() == self.n_age, "contact rows != n_age");
        anyhow::ensure!(
            self.contact.iter().all(|r| r.len() == self.n_age),
            "contact must be square n_age x n_age"
        );
        anyhow::ensure!(self.pop.len() == self.n_age, "pop.len != n_age");
        if let Some(v) = &self.vacc_rate {
            anyhow::ensure!(v.len() == self.n_age, "vacc_rate.len != n_age");
        }
        if let Some(v) = &self.aging_rate_per_day {
            anyhow::ensure!(v.len() == self.n_age, "aging_rate_per_day.len != n_age");
        }
        if let Some(v) = &self.fertility_per_day {
            anyhow::ensure!(v.len() == self.n_age, "fertility_per_day.len != n_age");
        }
        anyhow::ensure!(
            (0.0..=1.0).contains(&self.female_fraction),
            "female_fraction must be in [0,1]"
        );
        anyhow::ensure!(self.k_i >= 1, "k_i must be >= 1");
        anyhow::ensure!(self.mu >= 0.0 && self.mu_i_extra >= 0.0, "mu and mu_i_extra must be >= 0");
        Ok(())
    }

    pub fn state_size(&self) -> usize {
        self.n_age * (1 + self.k_i + 1)
    }
}

#[derive(Debug, Clone)]
pub struct SirState {
    pub y: Vec<f64>,
}

impl SirState {
    pub fn new_zero(cfg: &SirConfig) -> Self {
        Self {
            y: vec![0.0; cfg.state_size()],
        }
    }

    pub fn init_from_seeding(cfg: &SirConfig, seeding_per_age: &[f64]) -> Self {
        let mut s = Self::new_zero(cfg);
        for a in 0..cfg.n_age {
            let n = cfg.pop[a];
            let seed = seeding_per_age[a].min(n.max(0.0));
            let (idx_s, i0, _r) = indices(cfg, a);
            s.y[idx_s] = n - seed;
            s.y[i0] = seed;
        }
        s
    }
}

fn indices(cfg: &SirConfig, a: usize) -> (usize, usize, usize) {
    let block = 1 + cfg.k_i + 1;
    let base = a * block;
    let idx_s = base;
    let i0 = idx_s + 1;
    let r = i0 + cfg.k_i;
    (idx_s, i0, r)
}

pub struct SirModel {
    pub cfg: SirConfig,
}

impl SirModel {
    pub fn new(cfg: SirConfig) -> anyhow::Result<Self> {
        cfg.check()?;
        Ok(Self { cfg })
    }

    pub fn deriv(&self, t: f64, y: &[f64], dy: &mut [f64]) {
        dy.fill(0.0);
        let cfg = &self.cfg;

        let mut i_by_age = vec![0.0; cfg.n_age];
        let mut n_by_age = vec![0.0; cfg.n_age];
        for a in 0..cfg.n_age {
            let (s_idx, i0, r_idx) = indices(cfg, a);
            let s = y[s_idx];
            let r = y[r_idx];
            let mut i_sum = 0.0;
            for j in 0..cfg.k_i {
                i_sum += y[i0 + j];
            }
            i_by_age[a] = i_sum;
            n_by_age[a] = s + i_sum + r;
        }

        let beta = cfg.beta_at(t);
        let mut lambda = vec![0.0; cfg.n_age];
        for a in 0..cfg.n_age {
            let mut sum = 0.0;
            for b in 0..cfg.n_age {
                let nb = n_by_age[b];
                if nb > 0.0 {
                    sum += cfg.contact[a][b] * i_by_age[b] / nb;
                }
            }
            lambda[a] = beta * sum;
        }

        let zero;
        let vacc: &[f64] = if let Some(v) = &cfg.vacc_rate {
            v
        } else {
            zero = vec![0.0; cfg.n_age];
            &zero
        };

        let ki_gamma = (cfg.k_i as f64) * cfg.gamma;

        for a in 0..cfg.n_age {
            let (s_idx, i0, r_idx) = indices(cfg, a);

            let s = y[s_idx];
            let to_i = lambda[a] * s;
            let to_r_vacc = vacc[a] * s;
            dy[s_idx] -= to_i + to_r_vacc;
            dy[s_idx] -= cfg.mu * s;

            dy[i0] += to_i - ki_gamma * y[i0];
            dy[i0] -= (cfg.mu + cfg.mu_i_extra) * y[i0];
            for j in 1..cfg.k_i {
                dy[i0 + j] += ki_gamma * y[i0 + j - 1] - ki_gamma * y[i0 + j];
                dy[i0 + j] -= (cfg.mu + cfg.mu_i_extra) * y[i0 + j];
            }

            let inflow_r = ki_gamma * y[i0 + cfg.k_i - 1] + to_r_vacc;
            dy[r_idx] += inflow_r;
            dy[r_idx] -= cfg.mu * y[r_idx];
        }

        if let Some(fert) = &cfg.fertility_per_day {
            let ff = cfg.female_fraction;
            let mut births_per_day = 0.0;
            for a in 0..cfg.n_age {
                births_per_day += (n_by_age[a].max(0.0) * ff) * fert[a].max(0.0);
            }
            let (s0, _i0, _r0) = indices(cfg, 0);
            dy[s0] += births_per_day;
        }

        if let Some(aging) = &cfg.aging_rate_per_day {
            let block = 1 + cfg.k_i + 1;
            for a in 0..cfg.n_age.saturating_sub(1) {
                let rate = aging[a].max(0.0);
                if rate <= 0.0 {
                    continue;
                }
                let base_a = a * block;
                let base_b = (a + 1) * block;
                for off in 0..block {
                    let v = y[base_a + off];
                    let flow = rate * v;
                    dy[base_a + off] -= flow;
                    dy[base_b + off] += flow;
                }
            }
        }
    }

    pub fn simulate(&self, state: &mut SirState, t0: f64, t_end: f64, dt: f64) -> Vec<(f64, Vec<f64>)> {
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
