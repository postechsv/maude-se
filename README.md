# MaudeSE

MaudeSE extends [Maude](https://github.com/SRI-CSL/Maude) with SMT solving. It
supports satisfiability checks and symbolic search, with Python connectors for
Z3, Yices2, and cvc5. You can also write a connector for another solver.

## Install and run

The upcoming release supports Python 3.10–3.14 on macOS and Linux. Install
MaudeSE with Z3, its default solver:

```sh
python3 -m pip install 'maude-se[z3]'
```

This command applies after the upcoming release is published. Until then,
build and test the current source as described in [INSTALL.md](INSTALL.md).

This installs both MaudeSE and the Z3 Python package. The connectors and
converters are included in MaudeSE; the extra installs the solver package.

From a checkout of this repository, open one of the included examples:

```sh
maude-se examples/smt-check-ex.maude -s z3
```

At the `MaudeSE>` prompt, run `check in SIMPLE : X:Integer > 4 using QF_LRA .`;
the result should be `sat`.

To use Yices2 or cvc5 instead, install `maude-se[yices]` or `maude-se[cvc5]`
and select it with `-s yices` or `-s cvc5`. See the
[installation guide](INSTALL.md) for existing installations, standalone
executables, and source builds.

## Documentation

The [MaudeSE documentation](https://maude-se.github.io) covers commands,
examples, and the connector interface.
