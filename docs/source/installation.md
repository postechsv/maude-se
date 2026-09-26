# Installation

This page describes the MaudeSE 0.0.4 installation options: a Python package
and standalone executables. The Python package supports custom solver
connectors; standalone executables include a solver and do not require Python.

```{note}
The standalone executable uses its solver through a native C++ connection.
Python [connectors](https://github.com/postechsv/maude-se/tree/main/src/pysmt/connector)
and [converters](https://github.com/postechsv/maude-se/tree/main/src/pysmt/converter)
can be customized only with the Python package.
```

---

## MaudeSE Python Package

### Install with a solver

MaudeSE 0.0.4 supports Python 3.10–3.14 on macOS and Linux. Install it with
Z3, the default solver:

```sh
python3 -m pip install 'maude-se[z3]'
```

The extra installs the Z3 Python package; MaudeSE already includes its
connector and converter. For Yices2 or cvc5, use `maude-se[yices]` or
`maude-se[cvc5]` instead and select the solver with `-s yices` or `-s cvc5`.
The Yices2 extra installs both `yices` and `yices-solver`.

From a checkout of the repository, open an included example:

```sh
maude-se examples/smt-check-ex.maude -s z3
```

At the `MaudeSE>` prompt, run
`check in SIMPLE : X:Integer > 4 using QF_LRA .` and expect `result: sat`.

### Add a solver to an existing installation

If you installed the base `maude-se` package without a solver extra, add Z3
in that same Python environment:

```sh
maude-se-installer install z3
```

Installing `maude-se[z3]` is an alternative, not a preceding step. The
installer also accepts `yices`, `cvc5`, or `all` and can check the environment
with `maude-se-installer doctor`.

```{tip}
You can connect another SMT solver by implementing the MaudeSE *generic* interface.
See {ref}`generic`.
```

Existing release downloads are listed below. Check the wheel's Python and
platform tags before installing it. For local source builds, see
[INSTALL.md](https://github.com/postechsv/maude-se/blob/main/INSTALL.md).

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
