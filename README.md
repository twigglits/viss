| Badges     |  |
|------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Build      | [![Build Status](https://github.com/twigglits/viss/actions/workflows/c-cpp.yml/badge.svg?branch=main)](https://github.com/twigglits/viss/actions) |

### Viral Infection Simulation System (VISS)


**The latest build of this program is pre-Alpha and should not be used for any production or research purposes.**


VISS is an open-source simulation system for viral infections. It is a C++-based system that uses a combination of probabilistic models to study the behavior of viruses their growth and decay over time and their impact on human populations.

### Installing pre-requisites

```
sudo apt-get install -y cmake build-essential libgsl-dev libtiff-dev libboost-all-dev libhiredis-dev
```

### Getting Started

To get started with VISS, you will need to have a C++ compiler and CMake installed on your system. You can then clone the repository and build the system with the following  commands:

```bash
mkdir -p build && cd build && cmake .. && make -j4 && cd ..
```

For building a core set of binaries instead use:
```bash
mkdir -p build && cd build && cmake .. && make -j4 redis++ viss-release viss-api && cd ..
```

To run the program, you can use the following command:

```bash
./build/viss-release test_config1.txt 0 opt -o
```

For debugging our program it is:
```bash
./build/viss-debug test_config1.txt 0 opt -o
```

### Diseases

VISS supports a variety of diseases that can be used to study the impact of different interventions on viral infections. These diseases include:

- HIV
- HSV2

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

### Interventions

VISS supports a variety of interventions that can be used to study the impact of different interventions on viral infections. These interventions include:

- Antiretroviral therapy (ART)
- Circumcision
- Condom-use