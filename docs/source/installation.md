# Installation

MaudeSE is available as a Python package or a standalone executable. The
Python package supports custom solver connectors; standalone executables
include a solver and do not require Python.

```{note}
The standalone executable uses its solver through a native C++ connection.
Python [connectors](https://github.com/postechsv/maude-se/tree/main/src/pysmt/connector)
and [converters](https://github.com/postechsv/maude-se/tree/main/src/pysmt/converter)
can be customized only with the Python package.
```

---

## MaudeSE Python Package

### Install with a solver

For the published release, install MaudeSE and Z3, its default solver, in the
same Python environment. Published wheels cover Python 3.8–3.13 on macOS and
Linux:

```sh
python3 -m pip install maude-se 'z3-solver==4.13.0.0'
```

MaudeSE includes the Python connectors and converters for Z3, Yices2, and
cvc5, but the published release does not install a solver. To use Yices2 or
cvc5, install the corresponding solver Python package and select it with
`-s yices` or `-s cvc5`. Solver extras and `maude-se-installer` are available
in the current source build, but not yet in the published release.

From a checkout of the repository, open an included example:

```sh
maude-se examples/smt-check-ex.maude -s z3
```

At the `MaudeSE>` prompt, run
`check in SIMPLE : X:Integer > 4 using QF_LRA .` and expect `result: sat`.

### Add a solver to an existing installation

If you installed the published `maude-se` wheel without Z3, add it in that
same Python environment:

```sh
python3 -m pip install 'z3-solver==4.13.0.0'
```

For a wheel built from the current checkout, `pip install 'maude-se[z3]'`
installs both packages instead. That wheel also provides
`maude-se-installer install z3` for an existing base installation. These
commands are alternatives, not successive steps.

```{tip}
You can connect another SMT solver by implementing the MaudeSE *generic* interface.
See {ref}`generic`.
```

Published wheel downloads are listed below. Check the wheel's Python and
platform tags before installing it.

```{include} wheels.md
```

---

## Standalone Executable

Download a standalone executable for your platform from the releases below.
The listed executables include Z3 and do not need a Python installation.

```{include} native.md
```

For local source builds, including the Yices2 and cvc5 standalone variants,
see [INSTALL.md](https://github.com/postechsv/maude-se/blob/main/INSTALL.md).
