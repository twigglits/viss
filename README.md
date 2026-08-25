| Badges     |  |
|------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Build      | [![Build Status](https://github.com/twigglits/viss/actions/workflows/c-cpp.yml/badge.svg?branch=main)](https://github.com/twigglits/viss/actions) |

### Viral Infection Simulation System (VISS)


**The latest build of this program is pre-Alpha and should not be used for any production or research purposes.**


VISS is an open-source simulation system for viral infections. It is a Rust-based system that uses a combination of probabilistic models to study the behavior of viruses their growth and decay over time and their impact on human populations.

# VISS: Epidemiology Simulation in Rust

A fast, explainable, age-structured SEIRS ODE simulator with Erlang-stage compartments, designed to integrate with WorldPop population data and contact matrices.

## Installation

```bash
cargo clean && cargo update && cargo build --release
```

## Features
- Deterministic SEIRS with age structure and configurable Erlang stages (k_E, k_I)
- Stochastic SEIRS via the Gillespie direct-method SSA (exact CTMC), sharing the same config as the ODE
- Contact-matrix-based force of infection (Prem et al.-style)
- Piecewise-constant time-varying transmission multiplier m(t)
- Simple RK4 integrator for speed and determinism
- Calibration helper: compute beta0 for a target R0 using power iteration (spectral radius)
- CSV loaders for population by age and contact matrix
- Example: single-region, multi-age simulation
- Replayable run configs: every input of a hosted run, reproducible offline with `vrust_replay`

## Getting started
```bash
cargo run --release --bin single_region
```

## Data inputs
- Population CSV: header row, columns: `age_group,pop`
- Contact matrix CSV: square matrix with header row/col optional (will try to parse numeric cells)

These can be replaced later with aggregated WorldPop outputs.

## Reproducing a hosted run

A VISS deployment can hand you the exact input of any run it has done. The engine is open
source; the deployment around it need not be, so the config carries every input rather than
pointing at a database you cannot reach — rates, contact matrix, age pyramid, fertility and
aging schedules, initial seeding, and the RNG seed for stochastic runs.

```bash
# Download a run's input config and its output, then reproduce the run locally
curl -o input.json  https://<host>/api/run_config/<run_id>
curl -o hosted.csv  https://<host>/api/run_output/<run_id>

cargo run --bin vrust_replay -- input.json > local.csv
diff hosted.csv local.csv     # identical, including for seeded Gillespie runs
```

`vrust_replay` reads the config from a file or stdin and writes
`t_days,population,infected,incidence_pct` — the same columns the hosted run serves. It
touches no database and no network. A deterministic `seirs` config replays exactly; a
`gillespie` config replays exactly given its `rng_seed`, and is refused without one rather
than returning a different sample path that looks like a reproduction.

## Roadmap
- Add observation model (delayed NegBinon cases)
- Add multi-region coupling
- Add tests and benchmarking

## License
GNU GPL v3

### Diseases

VISS supports a variety of diseases that can be used to study the impact of different interventions on viral infections. These diseases include:

- HIV

### SEIRS model (mathematical definition)

One of the core deterministic compartmental models used in VISS is an SEIRS model with vaccination and demographic turnover.

Let:

- **S(t)** be the susceptible population
- **E(t)** be the exposed (infected but not yet infectious) population
- **I(t)** be the infectious population
- **R(t)** be the recovered/immune population
- **N(t) = S(t) + E(t) + I(t) + R(t)** be the total population

The model is defined by the ODE system:

```text
dS/dt = b (1 − ν) N − β (S I / N) − dS + αR − ρS
dE/dt = β (S I / N) − σE − dE
dI/dt = σE − γI − dI
dR/dt = b ν N + γI − dR − αR + ρS
```

Term definitions (typical units are per-day rates):

- **b**: per-capita birth rate (births occur at total rate bN)
- **d**: per-capita death rate (applied to all compartments)
- **β**: transmission rate parameter
- **σ**: latent progression rate (mean latent/incubation period is 1/σ)
- **γ**: recovery rate (mean infectious period is 1/γ)
- **α**: waning immunity rate (R → S)
- **ν**: fraction of newborns vaccinated at birth (births into R)
- **ρ**: vaccination rate applied to susceptibles (S → R)

The infection (incidence) term **β (S I / N)** corresponds to frequency-dependent transmission, i.e. the force of infection is λ(t) = β I/N and incidence is λS.

Summing the four equations gives dN/dt = (b − d) N. In particular, when b = d the total population remains constant.

### Stochastic SEIRS (Gillespie direct method)

The deterministic model above tracks expected compartment sizes. At small population
sizes, early in an outbreak, or when the question is "how often does this die out?",
the expectation is the wrong answer — VISS also provides the exact stochastic process
the ODE is the mean-field limit of.

The state is a vector of integer counts, and each ODE flow becomes a reaction channel
that moves exactly one individual. For the age-structured model with Erlang stages the
channels are:

| Channel | Transition | Propensity |
|---|---|---|
| Infection | S_a → E_{a,1} | λ_a(t) · S_a |
| Latent progression | E_{a,j} → E_{a,j+1} (last → I_{a,1}) | k_E σ · E_{a,j} |
| Infectious progression | I_{a,j} → I_{a,j+1} (last → R_a) | k_I γ · I_{a,j} |
| Waning immunity | R_a → S_a | ω · R_a |
| Vaccination | S_a → R_a | ρ_a · S_a |
| Death | X_a → ∅ | μ · X_a (plus μ_extra on I stages) |
| Birth | ∅ → S_1 | Σ_a f_a · ff · N_a |
| Aging | X_a → X_{a+1} | α_a · X_a |

with the same force of infection as the ODE, λ_a(t) = β(t) Σ_b C[a][b] I_b / N_b.

Given the total propensity a₀ = Σ_k a_k, Gillespie's direct method (1976/1977) draws

```text
τ ~ Exponential(a₀)                      time to the next event
P(channel k fires) = a_k / a₀            which event it is
```

then applies that channel's ±1 update and repeats. This is exact for the underlying
continuous-time Markov chain — not a discretisation of it — so the only approximation
is that a piecewise-constant β(t) is held fixed between events.

```rust
let mut model = vrust::GillespieModel::new(cfg, /* seed */ 42)?;
let mut state = vrust::SeirsState::init_from_seeding(&model.cfg, &seeding);
let trajectory = model.simulate(&mut state, 0.0, 365.0, 1.0);
```

Runs are seeded and reproducible, and the output is sampled on the same fixed grid as
`SeirsModel::simulate`, so a stochastic ensemble and the deterministic run can be
compared directly. Cost scales with the number of events, so whole-country populations
belong in the ODE; the SSA is for small populations, early-outbreak dynamics, and
fade-out probabilities.

### Interventions Roadmap

In the future, VISS will support a variety of interventions that can be used to study the impact of different interventions on viral infections. These interventions include:

- Pre-exposure prophylaxis (PrEP)
- Post-exposure prophylaxis (PEP)
- Antiretroviral therapy (ART)
- Vaccination
- Voluntary Male Circumcision (VMMC)
- Condom-use