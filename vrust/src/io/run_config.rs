//! The complete, self-contained description of one simulation run.
//!
//! Everything the engine needs is in here: the resolved `SeirsConfig` (rates, contact
//! matrix, age pyramid, fertility and aging schedules), which model consumed it, and the
//! seeding it started from. Nothing is looked up from a database at replay time, so a
//! config downloaded from a hosted VISS instance runs unchanged against a clone of this
//! repository:
//!
//! ```text
//! cargo run --bin vrust_replay -- input.json > output.csv
//! ```
//!
//! That is deliberate: the models are open source, the deployment around them need not be.

use serde::{Deserialize, Serialize};

use crate::model::gillespie::GillespieModel;
use crate::model::seirs::{SeirsConfig, SeirsModel, SeirsState};

/// Bump when a field changes meaning. Replaying an unknown version is an error rather
/// than a silently different run.
pub const SCHEMA_VERSION: u32 = 1;

pub const REPOSITORY: &str = "https://github.com/twigglits/viss";

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RunConfig {
    pub schema_version: u32,
    /// How to reproduce this run from a clone of `repository`.
    pub replay: String,
    pub repository: String,

    /// "seirs" (deterministic ODE) or "gillespie" (exact stochastic SSA).
    pub model: String,
    /// Provenance of the demography, not an input to the maths.
    pub iso3: String,
    pub year: i32,

    pub t_end_days: f64,
    pub dt_days: f64,
    /// SSA only; `None` for the deterministic model.
    pub rng_seed: Option<u64>,

    /// Initial infections per age bin, as handed to `SeirsState::init_from_seeding`.
    pub seeding: Vec<f64>,
    pub config: SeirsConfig,
}

impl RunConfig {
    pub fn new(
        model: &str,
        iso3: &str,
        year: i32,
        t_end_days: f64,
        dt_days: f64,
        rng_seed: Option<u64>,
        seeding: Vec<f64>,
        config: SeirsConfig,
    ) -> Self {
        Self {
            schema_version: SCHEMA_VERSION,
            replay: "cargo run --bin vrust_replay -- input.json > output.csv".to_string(),
            repository: REPOSITORY.to_string(),
            model: model.to_string(),
            iso3: iso3.to_string(),
            year,
            t_end_days,
            dt_days,
            rng_seed,
            seeding,
            config,
        }
    }

    /// Run it. Both models sample onto the same fixed `dt` grid, so the returned
    /// trajectory has the same shape either way.
    pub fn simulate(&self) -> anyhow::Result<Vec<(f64, Vec<f64>)>> {
        anyhow::ensure!(
            self.schema_version == SCHEMA_VERSION,
            "unsupported schema_version {} (this build understands {})",
            self.schema_version,
            SCHEMA_VERSION
        );

        let mut state = SeirsState::init_from_seeding(&self.config, &self.seeding);
        Ok(match self.model.as_str() {
            "gillespie" => {
                // An SSA replay without the original seed is a different sample path, not a
                // reproduction — refuse rather than hand back a lookalike.
                let seed = self
                    .rng_seed
                    .ok_or_else(|| anyhow::anyhow!("gillespie runs need rng_seed to be reproducible"))?;
                let mut m = GillespieModel::new(self.config.clone(), seed)?;
                m.simulate(&mut state, 0.0, self.t_end_days, self.dt_days)
            }
            "seirs" => {
                let m = SeirsModel::new(self.config.clone())?;
                m.simulate(&mut state, 0.0, self.t_end_days, self.dt_days)
            }
            other => anyhow::bail!("unknown model '{other}' (expected 'seirs' or 'gillespie')"),
        })
    }

    /// Compartment totals per grid point: `(t, susceptible, exposed, infected, recovered)`.
    pub fn totals(&self, traj: &[(f64, Vec<f64>)]) -> Vec<(f64, f64, f64, f64, f64)> {
        let cfg = &self.config;
        let block = 1 + cfg.k_e + cfg.k_i + 1;
        traj.iter()
            .map(|(t, y)| {
                let (mut s, mut e, mut i, mut r) = (0.0, 0.0, 0.0, 0.0);
                for a in 0..cfg.n_age {
                    let base = a * block;
                    s += y[base];
                    for j in 0..cfg.k_e {
                        e += y[base + 1 + j];
                    }
                    for j in 0..cfg.k_i {
                        i += y[base + 1 + cfg.k_e + j];
                    }
                    r += y[base + 1 + cfg.k_e + cfg.k_i];
                }
                (*t, s, e, i, r)
            })
            .collect()
    }

    /// The same three series the hosted API charts and stores, so a local replay can be
    /// diffed against a downloaded output config line for line.
    pub fn to_csv(&self, traj: &[(f64, Vec<f64>)]) -> String {
        let mut out = String::from("t_days,population,infected,incidence_pct\n");
        for (t, s, e, i, r) in self.totals(traj) {
            let denom = s + e + i + r;
            let incidence = if denom > 0.0 { (100.0 * i / denom).max(0.0) } else { 0.0 };
            out.push_str(&format!("{},{},{},{}\n", t, denom.ceil(), i.ceil(), incidence));
        }
        out
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::io::contact_synth::synthetic_contact_matrix;

    fn tiny(model: &str, rng_seed: Option<u64>) -> RunConfig {
        let n_age = 3;
        let cfg = SeirsConfig {
            n_age,
            k_e: 1,
            k_i: 1,
            sigma: 1.0 / 5.0,
            gamma: 1.0 / 10.0,
            omega: 0.0,
            mu: 0.0,
            mu_i_extra: 0.0,
            beta0: 0.4,
            beta_schedule: vec![(0.0, 1.0)],
            contact: synthetic_contact_matrix(n_age),
            pop: vec![1000.0, 1000.0, 1000.0],
            aging_rate_per_day: None,
            fertility_per_day: None,
            female_fraction: 0.5,
            vacc_rate: None,
        };
        RunConfig::new(model, "TST", 2025, 30.0, 0.5, rng_seed, vec![5.0, 5.0, 5.0], cfg)
    }

    /// The contract the download button sells: what a user gets back reproduces the run.
    #[test]
    fn a_serialised_config_replays_identically() {
        for cfg in [tiny("seirs", None), tiny("gillespie", Some(7))] {
            let here = cfg.simulate().unwrap();
            let json = serde_json::to_string(&cfg).unwrap();
            let there: RunConfig = serde_json::from_str(&json).unwrap();
            assert_eq!(there.simulate().unwrap(), here, "{} replay diverged", cfg.model);
            assert_eq!(there.to_csv(&here), cfg.to_csv(&here));
        }
    }

    #[test]
    fn an_ssa_config_without_a_seed_is_refused() {
        assert!(tiny("gillespie", None).simulate().is_err());
        assert!(tiny("nope", None).simulate().is_err());
    }
}
