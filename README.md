# MaudeSE

MaudeSE extends [Maude](https://github.com/SRI-CSL/Maude) with SMT solving. It
supports satisfiability checks and symbolic search, with Python connectors for
Z3, Yices2, and cvc5. You can also write a connector for another solver.

## Install and run

For the published release, install MaudeSE and Z3, its default solver, in the
same Python environment:

```sh
python3 -m pip install maude-se 'z3-solver==4.13.0.0'
```

Published wheels are available for Python 3.8–3.13 on macOS and Linux. The
current source build targets Python 3.10–3.14 and adds solver extras and the
`maude-se-installer` command; these are not yet in the published release.

From a checkout of this repository, open one of the included examples:

```sh
maude-se examples/smt-check-ex.maude -s z3
```

At the `MaudeSE>` prompt, run `check in SIMPLE : X:Integer > 4 using QF_LRA .`;
the result should be `sat`.

The connectors and converters for Z3, Yices2, and cvc5 are included in
MaudeSE. To use Yices2 or cvc5 instead, install the corresponding solver
Python package and select it with `-s yices` or `-s cvc5`. See
[installation and build instructions](INSTALL.md) for other installation
options, standalone executables, and local builds.

## Documentation

The [MaudeSE documentation](https://maude-se.github.io) covers commands,
examples, and the connector interface.
