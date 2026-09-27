# MaudeSE

MaudeSE extends [Maude](https://github.com/SRI-CSL/Maude) with SMT solving. It
supports satisfiability checks and symbolic search, with Python connectors for
Z3, Yices2, and cvc5. You can also write a connector for another solver.

## Install and run

The current release supports Python 3.10–3.14 on macOS and Linux. First,
install the base MaudeSE package from PyPI:

```sh
python3 -m pip install maude-se
```

The base package includes the solver connectors, but not a solver. Install Z3
in the same Python environment:

```sh
maude-se-installer install z3
```

Check that Z3 is available to MaudeSE:

```sh
maude-se-installer doctor z3
```

The example below requires a checkout of this repository; the PyPI package
does not include the `examples/` directory. From the repository root, open
an included example:

```sh
maude-se examples/smt-check-ex.maude -s z3
```

At the `MaudeSE>` prompt, enter:

```maude
check in SIMPLE : X:Integer > 4 using QF_LRA .
```

The result should be `sat`.

For other solvers, installation alternatives, and local builds, see
[INSTALL.md](INSTALL.md). The [website installation guide](https://maude-se.github.io/installation.html)
also covers release downloads and native solver plugins.

## Documentation

The [MaudeSE documentation](https://maude-se.github.io) covers commands,
examples, and the connector interface.
