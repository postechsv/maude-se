# Installation and source builds

## Install the Python package

The upcoming release supports Python 3.10–3.14 on macOS and Linux. Install
MaudeSE with Z3, the default solver:

```bash
python3 -m pip install 'maude-se[z3]'
```

This command applies after the upcoming release is published. To use the
current checkout now, build a wheel using the steps below.

MaudeSE already contains the connectors and converters for Z3, Yices2, and
cvc5. The `z3` extra installs the Z3 Python package; it does not install a
separate connector.

### Add a solver to an existing installation

The base wheel includes no solver Python package. If you installed `maude-se`
without an extra, add Z3 to that environment with:

```bash
maude-se-installer install z3
```

For a fresh installation with another solver, replace `[z3]` with `[yices]`
or `[cvc5]`; the Yices2 extra installs both `yices` and
`yices-solver`. For an existing installation, the installer accepts `yices`,
`cvc5`, or `all`. Run `maude-se-installer doctor` to check solver availability
without installing anything. Select the solver with `-s yices` or `-s cvc5`.

For a local wheel built using the steps below, install that wheel file with
its `[z3]` extra instead of fetching the package from PyPI.

### Run an example

From a checkout of this repository, run an included Maude file with the solver
you installed:

```bash
maude-se examples/smt-check-ex.maude -s z3
```

At the `MaudeSE>` prompt, run
`check in SIMPLE : X:Integer > 4 using QF_LRA .` and expect `result: sat`.

Use `maude-se --help` for command options. For a standalone executable that
includes its own solver, see the [release downloads](https://github.com/postechsv/maude-se/releases)
and the [website installation page](https://maude-se.github.io/installation.html).

## Build from source on macOS

The following steps are for contributors building wheels and standalone
executables locally. Finished artifacts are written to `out/`.

### 1. Check prerequisites

```bash
./build.sh doctor
```

`doctor` checks the local environment without changing it. The macOS build
requires Xcode Command Line Tools, Python 3.10–3.14, and Homebrew.

To include the additional standalone-build prerequisites in the check, run:

```bash
./build.sh doctor standalone
```

Wheel builds use these Homebrew tools:

- `bison`
- `flex`
- `cmake`
- `swig`

Standalone builds also need `autoconf` and `automake`, as well as `zip` and
`unzip` on your path. The other Homebrew tools above are shared.

To install missing Homebrew tools, run:

```bash
./build.sh install-deps
```

### 2. Prepare the build environment

```bash
./build.sh setup
```

This checks prerequisites, prepares the pinned upstream sources and patches,
and creates the build environment in `.build-wheel/cp<version>-<architecture>/venv-build`.
Each Python version and architecture keeps its own Maude sources, native Maude
build files, and virtual environments. Python-independent static dependencies
are built under `.build-wheel/dependencies/` and installed to
`.build-wheel/install/` for reuse by other Python versions. Changes to the
architecture, macOS deployment target, compiler, SDK, or dependency build
inputs trigger a rebuild. Switching the selected `python3` does not require
`clean`; `setup` reuses the matching environment. If the selected Python
installation changes within the same version, its build virtual environment
is recreated automatically.

### 3. Build the wheel

```bash
./build.sh wheel
```

This runs `setup`, builds MaudeSE and its native dependencies, and writes a
wheel to `out/`.

### 4. Test the wheel

```bash
./build.sh test-wheel
```

This selects the wheel in `out/` matching the selected `python3` and Mac
architecture, then recreates the corresponding
`.build-wheel/cp<version>-<architecture>/venv-test`. Multiple Python wheels
can coexist in `out/`. The test installs the solver extras and checks imports,
the installer, and SAT/UNSAT examples with Z3, Yices2, and cvc5. It also
checks the wheel's native library dependencies.

### 5. Build and test the standalone executable

```bash
./build.sh standalone
./build.sh test-standalone
```

`standalone` creates `out/maude_se_z3-<version>-macosx-<architecture>.zip`.
The executable includes Z3 and does not require the Python package.

`test-standalone` does not rebuild. It extracts the existing ZIP into a
temporary directory, runs a calculation plus Z3 SAT and UNSAT checks through
the packaged executable, and rejects non-system dynamic-library dependencies.

You can build and test a cvc5 variant separately:

```bash
./build.sh standalone cvc5
./build.sh test-standalone cvc5
```

This produces a ZIP containing `maude-se-cvc5` in `out/`.

To build the native Yices executable, or all three independent executables:

```bash
./build.sh standalone yices
./build.sh test-standalone yices
./build.sh standalone all
./build.sh test-standalone all
```

The Yices ZIP contains `maude-se-yices`. Each standalone variant includes its
own solver and can be run without installing a Python solver package. When
building multiple variants, common static libraries in
`.build-standalone/install/` are reused; only solver-specific dependencies and
the Maude executable are built separately. Changes to the architecture,
deployment target, compiler, SDK, or pinned dependency inputs trigger a
rebuild of the affected dependencies.

### 6. Build native solver plugins (optional)

Native plugins add a C++ solver connection to the Python wheel. They are
separate from the Python solver packages installed by `maude-se[z3]` and the
other extras. On macOS, `maude-se-installer install native z3` builds one
locally against the installed Maude library; it does not install a plugin
wheel. The installed package directory must be writable. See the
[native plugin build guide](src/plugins/README.md) for source and
build-directory options.

### 7. Use the installed development build

Open an isolated shell containing the tested MaudeSE installation:

```bash
./build.sh shell
```

This opens the selected Python's `.build-wheel/cp<version>-<architecture>/venv-test`.
Inside that shell, commands use the tested
installation:

```bash
maude-se --help
python -c "import maudeSE"
```

Run `exit` to return to the original shell.

To inspect the build tools instead, open the build environment:

```bash
./build.sh shell build
```

The build shell is intended for debugging CMake, Meson, Ninja, SWIG, or the
wheel build. It does not represent a clean MaudeSE installation.

### 8. Clean generated files

```bash
./build.sh clean
```

This removes the local build and test environments, downloaded upstream source,
native build products, and `out/`. It does not uninstall Homebrew packages.

For the complete local command list, run `./build.sh --help`.

## Release maintenance

The release version is defined in `src/pyproject.toml`. Wheel and standalone
artifact names and the MaudeSE banner derive from it during the build. A
release tag must use the matching `v<version>` form, such as `v0.0.3` for
version `0.0.3`.

Native dependencies are built from pinned sources. Their archives are checked
against the SHA-256 values in `build/versions.env`; update the pins together
when changing a dependency version.

### Test distribution builds in GitHub Actions

A collaborator with write access can open **Build and test** in GitHub Actions,
select **Run workflow**, and choose a pushed branch. This runs macOS and Linux
builds and tests without publishing. Build artifacts are available from the
completed workflow run. Unpushed local changes are not included.

The separate **Release** workflow runs the same build and tests when a
matching `v<version>` tag is pushed. It publishes wheels to PyPI unless the
tag contains `pre`, and publishes wheels plus standalone ZIPs to GitHub
Releases. PyPI publishing requires the repository's `pypi` environment and
trusted publisher to be configured.
