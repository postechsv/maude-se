# Installing and Building MaudeSE

## Install a released package

Install the latest released Python package from PyPI:

```bash
python3 -m pip install maude-se
```

MaudeSE selects Z3 by default, but the base wheel does not install any SMT
solver. The wheel installs `maude-se-installer`, which checks the solvers
available in the current Python environment:

```bash
maude-se-installer doctor
maude-se-installer doctor cvc5
```

Install Z3 for the default configuration, another solver of your choice, or
all solvers into that same environment:

```bash
maude-se-installer install z3
maude-se-installer install yices
maude-se-installer install cvc5
maude-se-installer install all
```

These commands use the installed MaudeSE version's declared dependencies.
Equivalent package extras are `maude-se[z3]`, `maude-se[yices]`,
`maude-se[cvc5]`, and `maude-se[all-solvers]`. Install Z3 before running
`maude-se` with its default configuration. The installer applies to the wheel;
the standalone executable includes its own solver.

Select a solver when running a Maude file:

```bash
maude-se example.maude -s cvc5
```

Check the installation with:

```bash
maude-se --help
```

## Change the MaudeSE version

The MaudeSE release version has one source of truth:
`src/pyproject.toml`. Change only its `version` field. Wheel names,
standalone ZIP names, Python package metadata, and the MaudeSE banner derive
their version from that value during the build. The banner build date is also
generated at build time, so building does not modify tracked source files.

Release tags must use the matching `v<version>` form. For example, version
`0.1.0` must be released with tag `v0.1.0`; the standalone release build fails
early when the tag and package version differ.

## Build locally on macOS

Generated build files are kept in two directories:

- `.build-wheel/` contains the wheel's external source checkouts, downloaded
  dependencies, installed native libraries, and Python environments.
- `.build-standalone/` contains the standalone Maude checkout, downloaded
  dependencies, and installed native libraries.

Finished wheels and standalone ZIP files are written to `out/`.

The wheel directory contains two isolated Python environments:

- `.build-wheel/venv-build` contains Meson, Ninja, and the Python packaging
  tools used to build the wheel. CMake and SWIG come from Homebrew.
- `.build-wheel/venv-test` contains the generated MaudeSE wheel, PyYAML, Z3, Yices, and
  cvc5. It
  represents a clean `all-solvers` user installation and is recreated by the
  test command.

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

The macOS build requires Xcode Command Line Tools, Python 3.10-3.14, and
Homebrew. Wheel builds use these Homebrew build tools:

- `bison`
- `flex`
- `cmake`
- `swig`

Standalone builds use Homebrew only for build tools:

- `bison`
- `flex`
- `autoconf`
- `automake`
- `cmake`

The native libraries used by the wheel and standalone builds are built from
pinned source versions instead of Homebrew bottles. Standalone builds include
a statically linked Z3, while wheels declare each pinned solver package as an
optional dependency. Source archives are checked against pinned SHA-256 hashes
before extraction; the standalone Z3 tag is also checked against a pinned
commit. Update `build/versions.env` hashes when intentionally changing a
dependency version.

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
2. creates or updates `.build-wheel/venv-build` with pinned build tools;
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
`out/`, recreates `.build-wheel/venv-test`, verifies the base wheel has no
solver, installs Z3 through the installer, then installs all supported solvers
and checks:

- `import maudeSE`;
- `maude-se --help`;
- `maude-se-installer doctor`;
- Z3, Yices, and cvc5 SAT/UNSAT smoke tests using
  `examples/smt-check-ex.maude`;
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
On macOS, all native dependencies in both artifact types use the same
deployment target: 10.13 for x86_64 and 11.0 for arm64. Override it with
`MAUDE_SE_MACOS_DEPLOYMENT_TARGET` when required.

`test-standalone` does not rebuild. It extracts the existing ZIP into a
temporary directory, runs a calculation plus Z3 SAT and UNSAT checks through
the packaged executable, and rejects non-system dynamic-library dependencies.

The native cvc5 standalone uses cvc5 1.4.0 and can be built separately:

```bash
./build.sh standalone cvc5
./build.sh test-standalone cvc5
```

This produces `out/maude_se_cvc5-<version>-<platform>-<architecture>.zip`
with the `maude-se-cvc5` executable. The cvc5 library is statically linked;
the build downloads a checksum-verified official cvc5 static distribution.

To build the native Yices executable, or all three independent executables:

```bash
./build.sh standalone yices
./build.sh test-standalone yices
./build.sh standalone all
./build.sh test-standalone all
```

The Yices build uses the checksum-verified Yices 2.6.5 static release,
statically links CUDD 3.0.0 from its pinned source revision, and uses the
libpoly static archive from the checksum-verified cvc5 distribution. Its ZIP
contains `maude-se-yices` and does not require a locally installed solver.

### 6. Use the installed development build

Open an isolated shell containing the tested MaudeSE installation:

```bash
./build.sh shell
```

This is equivalent to:

```bash
./build.sh shell test
```

Inside that shell, commands such as the following use `.build-wheel/venv-test`:

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
| `./build.sh setup` | Prepare pinned sources and `.build-wheel/venv-build` | Repository only |
| `./build.sh wheel` | Build a wheel into `out/` | Repository only |
| `./build.sh test` | Recreate `.build-wheel/venv-test` and test the existing wheel | Repository only |
| `./build.sh standalone` | Build a standalone executable ZIP into `out/` | Repository only |
| `./build.sh test-standalone` | Test the existing standalone ZIP | Temporary files only |
| `./build.sh shell` | Open the MaudeSE test environment | No persistent changes |
| `./build.sh shell build` | Open the build-tool environment | No persistent changes |
| `./build.sh clean` | Remove both build directories and `out/` | Repository only |

Display this command list at any time with:

```bash
./build.sh --help
```

## Test distribution builds in GitHub Actions

Once `.github/workflows/build.yml` is on the default branch, a repository
collaborator with write access can open **Build and test** in GitHub Actions,
select **Run workflow**, and choose a pushed branch. This runs the macOS and
Linux wheel and standalone builds and tests without publishing. Build
artifacts are available from the completed workflow run. Unpushed local
changes are not included.

The separate **Release** workflow runs the same build and tests when a
matching `v<version>` tag is pushed. It publishes wheels to PyPI unless the
tag contains `pre`, and publishes wheels plus standalone ZIPs to GitHub
Releases. PyPI publishing requires the repository's `pypi` environment and
trusted publisher to be configured.
