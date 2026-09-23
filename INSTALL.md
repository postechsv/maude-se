# Installing and Building MaudeSE

## Install a released package

Install the latest released Python package from PyPI:

```bash
python3 -m pip install maude-se
```

MaudeSE uses Z3 by default. Install its Python package to run the default
solver:

```bash
python3 -m pip install z3-solver
```

Check the installation with:

```bash
maude-se --help
```

## Build locally on macOS

The local build uses two isolated Python environments:

- `.venv-build` contains CMake, Meson, Ninja, SWIG, and the Python packaging
  tools used to build the wheel.
- `.venv-test` contains the generated MaudeSE wheel, PyYAML, and Z3. It
  represents a clean user installation and is recreated by the test command.

Both directories and all generated build artifacts are ignored by Git.

The standalone build does not use either Python environment. It produces a ZIP
containing the MaudeSE executable with Z3 statically linked and the required
Maude modules.

### 1. Check prerequisites

```bash
./build.sh doctor
```

This command only inspects the environment. It does not install or modify
anything.

To include the additional standalone-build prerequisites in the check, run:

```bash
./build.sh doctor standalone
```

The macOS build requires Xcode Command Line Tools, Python 3.8 or newer, and
Homebrew. The required Homebrew packages are:

- `bison`
- `flex`
- `gmp`
- `libsigsegv`
- `libtecla`
- `autoconf`
- `automake`
- `ncurses`

Install missing Homebrew packages explicitly with:

```bash
./build.sh install-deps
```

### 2. Prepare the build environment

```bash
./build.sh setup
```

This command:

1. checks the local prerequisites;
2. creates or updates `.venv-build` with pinned build tools;
3. checks out the pinned `maude-bindings` and `maudesmc` revisions; and
4. applies the MaudeSE patches if they have not already been applied.

It is safe to run `setup` repeatedly.

### 3. Build the wheel

```bash
./build.sh wheel
```

This runs `setup`, builds the native dependencies and Maude core, and writes
the resulting wheel to `out/`. The native Maude library is built in release
mode with compiler optimization, link-time optimization, and symbol stripping.

### 4. Test the wheel

```bash
./build.sh test
```

This command does not rebuild the wheel. It expects exactly one wheel in
`out/`, recreates `.venv-test`, installs the wheel and Z3, and checks:

- `import maudeSE`;
- `maude-se --help`; and
- the Z3-based `examples/smt-check-ex.maude` smoke test;
- native modules have no non-system dynamic-library dependencies.

On macOS, the Python extension and the operating system itself remain dynamic.
Third-party libraries are linked statically where supported; the wheel-bundled
`libmaude` reference is allowed because it is shipped inside the wheel.

To build and then test, run:

```bash
./build.sh wheel
./build.sh test
```

### 5. Build and test the standalone executable

```bash
./build.sh standalone
./build.sh test-standalone
```

`standalone` checks out the pinned Maude source, applies the native MaudeSE
patch, builds its native libraries, and creates
`out/maude_se_z3-<version>-macosx-<architecture>.zip`. The ZIP is independent
of the wheel and does not require a Python virtual environment. It keeps the
official Maude feature defaults and adds the MaudeSE SMT extension with a
statically linked Z3; the experimental integrated compiler remains disabled.

`test-standalone` does not rebuild. It extracts the existing ZIP into a
temporary directory, runs a calculation through the packaged executable, and
rejects non-system dynamic-library dependencies.

### 6. Use the installed development build

Open an isolated shell containing the tested MaudeSE installation:

```bash
./build.sh shell
```

This is equivalent to:

```bash
./build.sh shell test
```

Inside that shell, commands such as the following use `.venv-test`:

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

### 7. Clean generated files

```bash
./build.sh clean
```

This removes the local build and test environments, downloaded upstream source,
native build products, and `out/`. It does not uninstall Homebrew packages.

## Command summary

| Command | Purpose | Changes the system |
| --- | --- | --- |
| `./build.sh doctor` | Check macOS build prerequisites | No |
| `./build.sh doctor standalone` | Check wheel and standalone prerequisites | No |
| `./build.sh install-deps` | Install required Homebrew packages | Yes |
| `./build.sh setup` | Prepare pinned sources and `.venv-build` | Repository only |
| `./build.sh wheel` | Build a wheel into `out/` | Repository only |
| `./build.sh test` | Recreate `.venv-test` and test the existing wheel | Repository only |
| `./build.sh standalone` | Build a standalone executable ZIP into `out/` | Repository only |
| `./build.sh test-standalone` | Test the existing standalone ZIP | Temporary files only |
| `./build.sh shell` | Open the MaudeSE test environment | No persistent changes |
| `./build.sh shell build` | Open the build-tool environment | No persistent changes |
| `./build.sh clean` | Remove generated local build files | Repository only |

Display this command list at any time with:

```bash
./build.sh --help
```
