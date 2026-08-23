//! Stochastic SEIRS via Gillespie's direct-method SSA — the exact continuous-time
//! Markov chain whose mean-field limit is [`crate::model::seirs::SeirsModel`].
//!
//! Same [`SeirsConfig`], same compartment layout, same parameters: every flow in the
//! ODE right-hand side becomes an event that moves exactly one individual. Where the
//! ODE gives a smooth expectation, this gives one realisation — so it captures
//! stochastic fade-out, small-population noise, and the variance between outbreaks
//! that a deterministic run cannot.
//!
//! Time advances by an exponential waiting time drawn from the total event rate, and
//! the firing event is chosen proportional to its own rate (Gillespie 1976/1977).
//! The simulation is exact for the CTMC, not an approximation of it.

use crate::model::seirs::{indices, SeirsConfig, SeirsState};

/// PCG32 — small, seeded, reproducible.
///
/// ponytail: hand-rolled rather than pulling in `rand`; ~20 lines beats a new
/// dependency tree, and a fixed seed keeps runs reproducible like the ODE path.
pub struct Pcg32 {
    state: u64,
    inc: u64,
}

impl Pcg32 {
    pub fn new(seed: u64) -> Self {
        let mut r = Self { state: 0, inc: (seed << 1) | 1 };
        r.next_u32();
        r.state = r.state.wrapping_add(seed);
        r.next_u32();
        r
    }

    fn next_u32(&mut self) -> u32 {
        let old = self.state;
        self.state = old
            .wrapping_mul(6_364_136_223_846_793_005)
            .wrapping_add(self.inc);
        let xorshifted = (((old >> 18) ^ old) >> 27) as u32;
        let rot = (old >> 59) as u32;
        xorshifted.rotate_right(rot)
    }

    /// Uniform on the open interval (0, 1) — never returns 0, so `ln` stays finite.
    pub fn next_f64(&mut self) -> f64 {
        let hi = (self.next_u32() >> 5) as u64; // 27 bits
        let lo = (self.next_u32() >> 6) as u64; // 26 bits
        ((hi * 67_108_864 + lo) as f64 + 0.5) * (1.0 / 9_007_199_254_740_992.0)
    }
}

/// How an event's propensity is computed from the current state.
enum Rate {
    /// Per-capita rate times the source compartment's count.
    Linear(f64),
    /// Infection in the given age band: lambda_a(t) times the susceptible count.
    Infection(usize),
    /// Births, which depend on the whole state rather than one compartment.
    Birth,
}

/// One reaction channel: move an individual `from` one compartment `to` another.
/// `from = None` is a birth, `to = None` is a death.
struct Event {
    rate: Rate,
    from: Option<usize>,
    to: Option<usize>,
}

pub struct GillespieModel {
    pub cfg: SeirsConfig,
    events: Vec<Event>,
    rng: Pcg32,
}

impl GillespieModel {
    pub fn new(cfg: SeirsConfig, seed: u64) -> anyhow::Result<Self> {
        cfg.check()?;
        let events = build_events(&cfg);
        Ok(Self { cfg, events, rng: Pcg32::new(seed) })
    }

    /// Run one exact SSA trajectory from `t0` to `t_end`, recording the state on a
    /// fixed `dt` grid so the output matches `SeirsModel::simulate` and the two can
    /// be plotted or diffed against each other directly.
    ///
    /// The state is rounded to whole individuals on entry — the CTMC counts people,
    /// not fractions. Counts stay in `f64` to share `SeirsState` with the ODE; every
    /// value is an exact integer as long as populations stay under 2^53.
    pub fn simulate(
        &mut self,
        state: &mut SeirsState,
        t0: f64,
        t_end: f64,
        dt: f64,
    ) -> Vec<(f64, Vec<f64>)> {
        for v in state.y.iter_mut() {
            *v = v.round().max(0.0);
        }

        let n_steps = ((t_end - t0) / dt).round().max(0.0) as usize;
        let mut out = Vec::with_capacity(n_steps + 1);
        out.push((t0, state.y.clone()));

        let mut props = vec![0.0; self.events.len()];
        let mut t = t0;
        let mut k = 1usize;

        loop {
            let a0 = self.propensities(t, &state.y, &mut props);
            // Nothing left that can happen: coast to the end of the grid.
            let tau = if a0 > 0.0 {
                -self.rng.next_f64().ln() / a0
            } else {
                f64::INFINITY
            };
            let t_next = t + tau;

            // Record every grid point the system passes through before the event.
            let limit = t_next.min(t_end + 1e-9);
            while k <= n_steps {
                let ts = t0 + (k as f64) * dt;
                if ts > limit {
                    break;
                }
                out.push((ts, state.y.clone()));
                k += 1;
            }
            if k > n_steps || t_next >= t_end {
                break;
            }

            // Pick the firing channel proportional to its propensity.
            let mut u = self.rng.next_f64() * a0;
            let mut pick = props.len() - 1;
            for (i, p) in props.iter().enumerate() {
                u -= *p;
                if u <= 0.0 {
                    pick = i;
                    break;
                }
            }
            let ev = &self.events[pick];
            if let Some(i) = ev.from {
                state.y[i] -= 1.0;
            }
            if let Some(i) = ev.to {
                state.y[i] += 1.0;
            }
            t = t_next;
        }

        out
    }

    /// Fill `out` with every channel's propensity and return their sum.
    ///
    /// ponytail: linear scan over channels, O(events) per firing — fine up to a few
    /// hundred channels. Swap in a Fenwick tree or the next-reaction method
    /// (Gibson-Bruck) if age bands or stages grow enough to make this the bottleneck.
    fn propensities(&self, t: f64, y: &[f64], out: &mut [f64]) -> f64 {
        let cfg = &self.cfg;

        // I_a and N_a at time t (local allocations, matching SeirsModel::deriv)
        let mut i_by_age = vec![0.0; cfg.n_age];
        let mut n_by_age = vec![0.0; cfg.n_age];
        for a in 0..cfg.n_age {
            let (s_idx, e0, i0, r_idx) = indices(cfg, a);
            let mut e_sum = 0.0;
            let mut i_sum = 0.0;
            for j in 0..cfg.k_e {
                e_sum += y[e0 + j];
            }
            for j in 0..cfg.k_i {
                i_sum += y[i0 + j];
            }
            i_by_age[a] = i_sum;
            n_by_age[a] = y[s_idx] + e_sum + i_sum + y[r_idx];
        }

        // Force of infection per age, identical to the deterministic model.
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

        let births: f64 = match &cfg.fertility_per_day {
            Some(fert) => (0..cfg.n_age)
                .map(|a| n_by_age[a].max(0.0) * cfg.female_fraction * fert[a].max(0.0))
                .sum(),
            None => 0.0,
        };

        let mut total = 0.0;
        for (i, e) in self.events.iter().enumerate() {
            let r = match (&e.rate, e.from) {
                (Rate::Linear(c), Some(src)) => c * y[src],
                (Rate::Infection(a), Some(src)) => lambda[*a] * y[src],
                (Rate::Birth, _) => births,
                _ => 0.0,
            }
            .max(0.0);
            out[i] = r;
            total += r;
        }
        total
    }
}

/// Enumerate every reaction channel once, up front. Channels with a zero constant
/// rate (no waning, no vaccination, no mortality) are dropped rather than carried
/// as permanent zeros.
fn build_events(cfg: &SeirsConfig) -> Vec<Event> {
    let mut ev = Vec::new();
    let ke_sigma = (cfg.k_e as f64) * cfg.sigma;
    let ki_gamma = (cfg.k_i as f64) * cfg.gamma;
    let block = 1 + cfg.k_e + cfg.k_i + 1;

    let mut push = |rate: Rate, from: Option<usize>, to: Option<usize>| {
        if let Rate::Linear(c) = rate {
            if c <= 0.0 {
                return;
            }
        }
        ev.push(Event { rate, from, to });
    };

    for a in 0..cfg.n_age {
        let (s, e0, i0, r) = indices(cfg, a);

        // Infection: S -> E1
        push(Rate::Infection(a), Some(s), Some(e0));

        // Latent stage progression, last E stage feeding I1
        for j in 0..cfg.k_e {
            let to = if j + 1 < cfg.k_e { e0 + j + 1 } else { i0 };
            push(Rate::Linear(ke_sigma), Some(e0 + j), Some(to));
        }

        // Infectious stage progression, last I stage feeding R
        for j in 0..cfg.k_i {
            let to = if j + 1 < cfg.k_i { i0 + j + 1 } else { r };
            push(Rate::Linear(ki_gamma), Some(i0 + j), Some(to));
        }

        // Waning immunity R -> S, and vaccination S -> R
        push(Rate::Linear(cfg.omega), Some(r), Some(s));
        let vacc = cfg.vacc_rate.as_ref().map_or(0.0, |v| v[a].max(0.0));
        push(Rate::Linear(vacc), Some(s), Some(r));

        // Deaths out of every compartment, with excess mortality on the I stages
        let base = a * block;
        for off in 0..block {
            let idx = base + off;
            let infectious = (i0..i0 + cfg.k_i).contains(&idx);
            let mu = if infectious { cfg.mu + cfg.mu_i_extra } else { cfg.mu };
            push(Rate::Linear(mu), Some(idx), None);
        }
    }

    // Births enter the youngest susceptible compartment
    if cfg.fertility_per_day.is_some() {
        let (s0, _, _, _) = indices(cfg, 0);
        push(Rate::Birth, None, Some(s0));
    }

    // Aging moves individuals up a band without changing their disease state
    if let Some(aging) = &cfg.aging_rate_per_day {
        for a in 0..cfg.n_age.saturating_sub(1) {
            let rate = aging[a].max(0.0);
            let (base_a, base_b) = (a * block, (a + 1) * block);
            for off in 0..block {
                push(Rate::Linear(rate), Some(base_a + off), Some(base_b + off));
            }
        }
    }

    ev
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::calibration::beta0_from_r0;
    use crate::model::seirs::SeirsModel;

    /// Single age band, no demography — the classic closed SEIR.
    fn closed_seir(pop: f64, r0: f64) -> SeirsConfig {
        let gamma = 1.0 / 5.0;
        let contact = vec![vec![1.0]];
        SeirsConfig {
            n_age: 1,
            k_e: 1,
            k_i: 1,
            sigma: 1.0 / 3.0,
            gamma,
            omega: 0.0,
            mu: 0.0,
            mu_i_extra: 0.0,
            beta0: beta0_from_r0(&contact, gamma, r0),
            beta_schedule: vec![],
            contact,
            pop: vec![pop],
            aging_rate_per_day: None,
            fertility_per_day: None,
            female_fraction: 0.5,
            vacc_rate: None,
        }
    }

    #[test]
    fn same_seed_gives_same_trajectory() {
        let cfg = closed_seir(10_000.0, 2.5);
        let run = |seed| {
            let mut m = GillespieModel::new(cfg.clone(), seed).unwrap();
            let mut st = SeirsState::init_from_seeding(&cfg, &[50.0]);
            m.simulate(&mut st, 0.0, 120.0, 1.0)
        };
        assert_eq!(run(7), run(7), "SSA must be reproducible for a fixed seed");
        assert_ne!(run(7), run(8), "different seeds must diverge");
    }

    #[test]
    fn counts_stay_whole_and_population_is_conserved() {
        let cfg = closed_seir(20_000.0, 2.0);
        let mut m = GillespieModel::new(cfg.clone(), 42).unwrap();
        let mut st = SeirsState::init_from_seeding(&cfg, &[20.0]);
        for (t, y) in m.simulate(&mut st, 0.0, 200.0, 1.0) {
            let total: f64 = y.iter().sum();
            assert!(
                (total - 20_000.0).abs() < 1e-9,
                "population drifted to {total} at t={t}"
            );
            for v in &y {
                assert!(*v >= 0.0 && v.fract() == 0.0, "non-integer count {v} at t={t}");
            }
        }
    }

    /// The whole point of an exact SSA: averaged over realisations it must reproduce
    /// the deterministic model it is the stochastic counterpart of.
    #[test]
    fn mean_final_size_matches_the_ode() {
        let (pop, r0) = (50_000.0, 2.5);
        let cfg = closed_seir(pop, r0);
        let recovered = |y: &[f64]| *y.last().unwrap();

        let ode = SeirsModel::new(cfg.clone()).unwrap();
        let mut st = SeirsState::init_from_seeding(&cfg, &[100.0]);
        let det = recovered(&ode.simulate(&mut st, 0.0, 365.0, 0.25).last().unwrap().1) / pop;

        let runs = 8;
        let mut mean = 0.0;
        for seed in 0..runs {
            let mut m = GillespieModel::new(cfg.clone(), seed).unwrap();
            let mut st = SeirsState::init_from_seeding(&cfg, &[100.0]);
            mean += recovered(&m.simulate(&mut st, 0.0, 365.0, 1.0).last().unwrap().1) / pop;
        }
        mean /= runs as f64;

        assert!(det > 0.5, "deterministic run should produce a major epidemic, got {det}");
        assert!(
            (mean - det).abs() / det < 0.03,
            "SSA mean attack rate {mean} strayed from the ODE final size {det}"
        );
    }
}
