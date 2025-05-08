MaudeSE is an SMT extension of [Maude](https://github.com/SRI-CSL/Maude) that supports various SMT features, including symbolic SMT search and satisfiability checking for SMT formulas.

It provides a *generic interface* that enables Maude to connect with SMT solvers. Users can implement their own connectors to integrate Maude with their preferred SMT solvers. Currently, MaudeSE includes built-in interfaces for Z3, Yices2, and CVC5.

For details on implementing a custom connector or using MaudeSE, please refer to our [webpage](https://maude-se.github.io).


## Prerequisite

* MaudeSE requires `Python >= 3.8`.
* Python supported SMT solvers that you want to use. 
  * E.g., `z3`, `yices`, `cvc5`.

## Installation

Use `pip` to install `maude-se`

```
pip install maude-se
```

Use the following command to test successful installation.

```
$ maude-se -h
```

If the installation was successful, you can see the following message.

```
usage: maude-se [-h] [-cfg CONFIG] [-s SOLVER] [-no-meta] [file]

positional arguments:
  file                  input Maude file

options:
  -h, --help            show this help message and exit
  -cfg CONFIG, -config CONFIG
                        a directory to a configuration file (default: "config.yml")
  -s SOLVER, -solver SOLVER
                        solver name
  -no-meta              no metaInterpreter
```