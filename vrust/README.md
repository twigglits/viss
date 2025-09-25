# vrust: Epidemiology Simulation in Rust

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
