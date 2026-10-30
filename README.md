# C++ Circuit Simulator

A C++-based circuit simulation program developed as a collaborative undergraduate project. The simulator implements circuit analysis using **Modified Nodal Analysis (MNA)** and supports both linear and nonlinear circuit elements.

## Overview

This project is a lightweight circuit simulation engine written in C++. It parses circuit descriptions from a netlist, constructs the corresponding system of equations, and performs numerical circuit analysis.

The implementation focuses on the core concepts behind SPICE-like circuit simulation, including matrix-based circuit formulation, transient analysis, and nonlinear diode modeling.

## Features

- Modified Nodal Analysis (MNA)
- DC circuit analysis
- Transient analysis
- Resistor (`R`)
- Capacitor (`C`)
- Inductor (`L`)
- Independent DC voltage sources
- Sinusoidal voltage sources
- Ideal diode model
- Zener diode model
- Netlist parsing
- Engineering suffix support
  - `k` — kilo
  - `m` — milli
  - `u` — micro
  - `n` — nano
  - `p` — pico
- Dynamic construction of circuit matrices
- Support for circuits with different numbers of nodes and elements
- Numerical solution of the resulting system of equations

## Numerical Approach

The simulator uses **Modified Nodal Analysis (MNA)** to formulate the circuit equations.

For each circuit, the program:

1. Parses the input netlist.
2. Identifies circuit nodes and elements.
3. Constructs the MNA system.
4. Builds the corresponding coefficient matrix and right-hand-side vector.
5. Solves the resulting system numerically.
6. Performs repeated analysis for transient simulations.
7. Updates nonlinear diode states during simulation.

This approach provides a foundation for implementing the main numerical components of a SPICE-like circuit simulator.

## Supported Circuit Elements

| Element | Description |
|---|---|
| `R` | Resistor |
| `C` | Capacitor |
| `L` | Inductor |
| `V` | Independent voltage source |
| `D` | Ideal diode |
| `Z` | Zener diode |

The simulator also supports sinusoidal voltage sources for time-domain analysis.

## Example Netlist

A circuit can be described using a netlist containing component definitions and their connections.

Example:

```text
V1 1 0 DC 5
R1 1 2 1k
R2 2 0 2k
```

The simulator parses the circuit description and automatically constructs the corresponding system of equations.

## Project Structure

```text
cpp-circuit-simulator/
│
├── src/
│   └── circuit_simulator.cpp
│
├── CMakeLists.txt
├── .gitignore
└── README.md
```

## Technologies

- **C++**
- **CMake**
- Modified Nodal Analysis
- Numerical Linear Algebra
- Circuit Analysis
- Transient Simulation
- Netlist Parsing

## Skills Demonstrated

This project demonstrates practical experience with:

- Numerical modeling
- Circuit simulation algorithms
- Matrix-based problem solving
- Object-oriented programming in C++
- Parsing structured input
- Implementation of circuit-analysis methods
- Handling nonlinear circuit elements
- Debugging and testing numerical software

## Project Context

This simulator was developed collaboratively as an undergraduate course project.

The project involved implementing the main components of a circuit simulation engine from the underlying circuit-analysis equations through numerical simulation.

## Contributors

- Avin Salmanpour
- Project collaborator

## License

This project was developed for educational purposes.
