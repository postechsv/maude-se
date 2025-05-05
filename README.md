MaudeSE is an SMT extension of [Maude](https://github.com/SRI-CSL/Maude). It provides a symbolic SMT search and satisfiability checking for SMT formulas. Currently supported SMT solvers are Z3, Yices2, and CVC5.

## Prerequisite

* MaudeSE requires `Python >= 3.8`.
* Among the supported solvers, the Yices2 Python binding requires the Yices executable to be installed. 
  * [https://github.com/SRI-CSL/yices2](https://github.com/SRI-CSL/yices2)


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
usage: maude-se [-h] [-s [SOLVER]] [-no-meta] [file]

positional arguments:
  file                  input Maude file

options:
  -h, --help            show this help message and exit
  -s [SOLVER], -solver [SOLVER]
                        set an underlying SMT solver
                        * Supported solvers: {z3,yices,cvc5}
                        * Usage: -s cvc5
                        * Default: -s z3
  -no-meta              no metaInterpreter
```