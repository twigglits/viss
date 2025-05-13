### Viral Infection Simulation System (VISS)

```
The latest build of this program is pre-Alpha and should not be used for any production or research purposes.
```

VISS is a simulation system for viral infections. It is a C++-based system that uses a combination of probabilistic models to study the behavior of viruses their growth and decay over time and their impact on human populations.

### Getting Started

To get started with VISS, you will need to have a C++ compiler and CMake installed on your system. You can then clone the repository and build the system with the following  commands:

```bash
mkdir -p build && cd build && cmake .. && make -j4 && cd ..
```

To run the program, you can use the following command:

```bash
./build/viss-release test_config1.txt 0 opt -o
```

For debugging our program it is:
```bash
./build/viss-debug test_config1.txt 0 opt -o
```
