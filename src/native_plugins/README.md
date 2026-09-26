# Building native solver plugins

These optional plugins use a native C++ connection to Z3, Yices2, or cvc5.
They are for local testing and have not been published to a package index.
They are separate from the Python solver packages installed by extras such as
`maude-se[z3]`.
The base `maude-se` wheel contains Maude. The installed
`maude-se-installer install native NAME` command now compiles a connector
locally, placing it and its checked solver library under the installed
`maudeSE/native/NAME` directory. No separately published plugin wheel is needed.

For a local macOS build, first build the base wheel:

```sh
./build.sh wheel
```

To test in another Python environment, install the locally built base wheel
there first. The `out/` directory may contain wheels for several Python
versions; select the current one:

```sh
python_tag="$(python3 -c 'import sys; print(f"cp{sys.version_info.major}{sys.version_info.minor}")')"
arch="$(uname -m)"
python3 -m pip install out/maude_se-*-"$python_tag"-"$python_tag"-macosx_*_"$arch".whl
maude-se-installer install native all --source-dir .
maude-se-installer doctor native
maude-se model.maude -s z3 -native
```

Without `--source-dir`, the installer clones the `v<installed maude-se version>`
source tag to `maudeSE/.native-build/maude-se`. The tag must exist upstream.
Use `--build-dir DIRECTORY` to keep that clone elsewhere. This process needs
the macOS build prerequisites listed in `INSTALL.md`, network access for
pinned dependencies, and write access to the installed package directory.
The installer verifies the solver shared library's SHA-256 checksum. For an
offline test with one solver, pass `--asset-archive FILE`. Local source builds
currently support macOS only. The selected Yices wheel requires macOS 14
on arm64 or macOS 13 on x86_64.

`maude-se-installer uninstall native all` removes the three native plugins.
`maude-se-installer uninstall all` removes the Python solver packages. Neither
command removes `maude-se`.

Each locally built plugin links against the installed Maude library. The loader
checks the plugin ABI and solver identity; there is no separate plugin package
version check.
