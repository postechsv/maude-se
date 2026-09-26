# Optional native solver plugins

The base `maude-se` wheel contains the Maude engine and the Python connector
interface, but no native SMT solver. Each native plugin wheel contains one
Maude-SE converter/connector and links to the base wheel's `libmaude`.
The three plugin wheels do not bundle solver binaries. The installer fetches
checksum-pinned shared libraries into each plugin's `solver/` directory. Z3
and cvc5 come from their upstream releases (the Linux Z3 library comes from
the upstream-published PyPI wheel). Yices comes from the same `yices-solver`
PyPI wheel used by the Python backend; that wheel is published separately from
the SRI Yices release archive.

For a local macOS build, prepare the pinned standalone dependencies first,
then build the base wheel and the plugin wheels:

```sh
./build.sh standalone all
./build.sh wheel
./build.sh plugin all
```

Install the base wheel, then use the installer with locally built wheels:

```sh
python -m pip install out/maude_se-0.0.3-*.whl
maude-se-installer install native all --find-links out
maude-se-installer doctor native
maude-se model.maude -s z3 -native
maude-se-installer uninstall native z3
```

Without `--find-links`, the installer requests `maude-se-native-<solver>` at
exactly the installed Maude-SE version from the configured package index. It
then downloads the matching solver archive, verifies its SHA-256, and extracts
only shared libraries and license files. For an offline local test, use
`--asset-archive FILE` with one native solver. The
plugins have not yet been published to an index. Local validation has covered
macOS arm64 with Python 3.12; Linux, Intel macOS, and the other supported
Python versions still need build and runtime verification. The selected Yices
wheel requires macOS 14 on arm64 or macOS 13 on x86_64; other plugin wheels
may support older macOS versions.

`uninstall native all` removes all three native plugin packages, while
`uninstall all` removes the Python solver packages. Neither removes `maude-se`.

The plugin ABI number and solver identity are checked when loading, and the
plugin binary is kept loaded for the lifetime of the process. A matching
package version is required because the plugin uses Maude's C++ ABI. Before
public distribution, audit every wheel's linked dependencies and license
notices.
