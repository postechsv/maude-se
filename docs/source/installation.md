# Installation

MaudeSE can be installed in two ways: (i) as a Python package, or (ii) as a standalone executable.
If you are looking for an older version, you can find it {doc}`here <old>`.

```{note}
The standalone executable tightly integrates with its underlying SMT solver at the C++ level, and is more efficient than the Python package. 
However, its [connector](https://github.com/postechsv/maude-se/tree/main/src/pysmt/connector) and [converter](https://github.com/postechsv/maude-se/tree/main/src/pysmt/converter) cannot be extended or customized at the Python level.
```

---

## MaudeSE Python Package

### Wheel Installation

Use `pip` to install the latest version of the MaudeSE Python package from PyPi.

```console
$ pip install maude-se
```

```{important}
Python version 3.8 or newer is required.
```

You can also manually install the MaudeSE wheel that matches your operating system (`macOS` or `Linux`) and machine architecture (`arm64` or `x86_64`) from
our GitHub repository:

* [https://github.com/postechsv/maude-se/releases](https://github.com/postechsv/maude-se/releases)

```console
$ pip install ./maude_se-<MAUDE_SE_VERSION>-cp<PYTHON_VERSION>-cp<PYTHON_VERSION>-<OS>_<ARCH>.whl
```

```{include} wheels.md
```

### Smoke Test 

Use the following command to test successful installation.

```console
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

### SMT Solvers

MaudeSE is shipped with [connectors](https://github.com/postechsv/maude-se/tree/main/src/pysmt/connector) and [converters](https://github.com/postechsv/maude-se/tree/main/src/pysmt/converter) for integrating various SMT solvers (Z3, Yices2, and CVC5), but not containing the solvers. 
You need to download SMT solvers and their Python bindings by yourself. 

* [https://github.com/Z3Prover/z3](https://github.com/Z3Prover/z3)
* [https://github.com/SRI-CSL/yices2](https://github.com/SRI-CSL/yices2)
* [https://github.com/cvc5/cvc5](https://github.com/cvc5/cvc5)

For example, if you want to use MaudeSE with Z3, use the following command to install the Z3 Python package along with all required dependencies:

```console
$ pip install z3-solver
```

```{tip}
You can also connect other SMT solvers to MaudeSE by implementing the MaudeSE *generic* interface.
See {ref}`generic`.
```

---

## Standalone Executable

Currently, we provide standalone executables for macOS and Linux, statically linked with Z3.

```{include} native.md
```

```{note}
Standalone executables for Yices2 and CVC5 will be updated soon.
```