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
- Contact-matrix-based force of infection (Prem et al.-style)
- Piecewise-constant time-varying transmission multiplier m(t)
- Simple RK4 integrator for speed and determinism
- Calibration helper: compute beta0 for a target R0 using power iteration (spectral radius)
- CSV loaders for population by age and contact matrix
- Example: single-region, multi-age simulation

## Getting started
```bash
cargo run --release --bin single_region
```

## Data inputs
- Population CSV: header row, columns: `age_group,pop`
- Contact matrix CSV: square matrix with header row/col optional (will try to parse numeric cells)

These can be replaced later with aggregated WorldPop outputs.

## Roadmap
- Add stochastic CTMC (Gillespie)
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

### Interventions Roadmap

In the future, VISS will support a variety of interventions that can be used to study the impact of different interventions on viral infections. These interventions include:

- Pre-exposure prophylaxis (PrEP)
- Post-exposure prophylaxis (PEP)
- Antiretroviral therapy (ART)
- Vaccination
- Voluntary Male Circumcision (VMMC)
- Voluntary Female Circumcision (VFC)
- Condom-use