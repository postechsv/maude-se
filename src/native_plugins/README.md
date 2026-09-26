# Building native solver plugins

These optional plugins use a native C++ connection to Z3, Yices2, or cvc5.
They are for local testing and have not been published to a package index.
They are separate from the Python solver packages installed by extras such as
`maude-se[z3]`.
The base `maude-se` wheel contains Maude; each plugin wheel contains one
connector and requires a matching version of the base package.

For a local macOS build, prepare the pinned standalone dependencies first,
then build the base wheel and the plugin wheels:

```sh
./build.sh standalone all
./build.sh wheel
./build.sh plugin all
```

Install the locally built base wheel, then the plugins. The `out/` directory
should contain one base wheel for your Python version and platform:

```sh
python -m pip install out/maude_se-*.whl
maude-se-installer install native all --find-links out
maude-se-installer doctor native
maude-se model.maude -s z3 -native
```

The installer gets each solver's shared library separately, verifies its
SHA-256 checksum, and installs it alongside the plugin. For an offline test
with one solver, pass `--asset-archive FILE`. Local validation has covered
macOS arm64 with Python 3.12; Linux, Intel macOS, and other Python versions
still need runtime verification. The selected Yices wheel requires macOS 14
on arm64 or macOS 13 on x86_64.

`maude-se-installer uninstall native all` removes the three native plugins.
`maude-se-installer uninstall all` removes the Python solver packages. Neither
command removes `maude-se`.

Each plugin uses Maude's C++ ABI, so its package version must match the base
package. The loader also checks the plugin ABI and solver identity.
