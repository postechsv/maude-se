# Installation

The recommended path is to install MaudeSE from PyPI, add an SMT solver, and
run an example. Other options below cover pip extras, downloadable wheels, and
standalone executables. If you need native C++ solver support with the Python
package, see {ref}`native-solver-plugins`.

---

## Install from PyPI

The current Python package supports Python 3.10–3.14 on macOS and Linux.
First, install MaudeSE:

```sh
python3 -m pip install maude-se
```

MaudeSE includes `maude-se-installer`, a command for adding optional SMT
solver packages to the same Python environment and checking whether they are
ready. MaudeSE already includes the connectors and converters, but it does not
install a solver package by default. Add Z3, the default solver, with:

```sh
maude-se-installer install z3
```

To check which solvers are ready, run `maude-se-installer doctor`.

```{tip}
To add another solver, use `maude-se-installer install yices` or
`maude-se-installer install cvc5`. Use `maude-se-installer install all` for all
three. The Yices2 option installs both `yices` and `yices-solver`.
```

### Run an example

From a checkout of the repository, open an included example:

```sh
maude-se examples/smt-check-ex.maude -s z3
```

The `-s` option selects a solver; use `-s yices` or `-s cvc5` if you installed
one of those instead.

At the `MaudeSE>` prompt, check a formula:

```maude
MaudeSE> check in SIMPLE : X:Integer > 4 using QF_LRA .
result: sat
```

---

## Other options

### Install with a pip extra

You can install MaudeSE and Z3 in one step instead:

```sh
python3 -m pip install 'maude-se[z3]'
```

This replaces the two installation commands above; there is no need to run
`maude-se-installer install z3` afterward. The `yices` and `cvc5` extras are
also available.

### Install from a release wheel

You can install MaudeSE from a downloaded wheel instead of using the package
index. Choose a wheel below that matches your Python version and platform.
With the downloaded wheel in your current directory, install that file with
`pip` (replace the filename with the one you downloaded):

```sh
python3 -m pip install ./maude_se-VERSION-PYTHON-PLATFORM.whl
```

For a wheel that includes `maude-se-installer`, add a solver as described
above. The table includes older releases, so check the instructions for the
release you download. For local source builds, see
[INSTALL.md](https://github.com/postechsv/maude-se/blob/main/INSTALL.md).

```{include} wheels.md
```

### Standalone executable

Download a standalone executable for your platform from the releases below.
The listed executables include Z3 and do not need a Python installation. They
use a native C++ solver connection; custom Python
[connectors](https://github.com/postechsv/maude-se/tree/main/src/pysmt/connector)
and [converters](https://github.com/postechsv/maude-se/tree/main/src/pysmt/converter)
require the Python package instead.

```{include} native.md
```

For local source builds, including the Yices2 and cvc5 standalone variants,
see [INSTALL.md](https://github.com/postechsv/maude-se/blob/main/INSTALL.md).

---

(native-solver-plugins)=
## Native solver plugins

For an installed MaudeSE Python package, you can optionally use a native C++
solver plugin instead of a Python solver connection. Unlike a standalone
executable, this still uses the Python package. It does not require the
corresponding Python solver package.

On macOS, build and install a Z3 plugin for the installed MaudeSE version with:

```sh
maude-se-installer install native z3
```

This builds the plugin locally. It requires the native build prerequisites,
network access to obtain the matching MaudeSE source and solver dependencies,
and write access to the installed Python package. Check the plugin and select
it when running MaudeSE:

```sh
maude-se-installer doctor native z3
maude-se examples/smt-check-ex.maude -s z3 -native
```

The installer also accepts `yices`, `cvc5`, or `all` after `native`. For build
prerequisites and local-source options, see the
[native plugin guide](https://github.com/postechsv/maude-se/blob/main/src/plugins/README.md).
