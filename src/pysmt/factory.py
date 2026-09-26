"""Register Python or separately installed native SMT backends."""

import importlib
from importlib import metadata

from maudeSE.maude import (
    install_python_smt_factory,
    loadNativeSmtPlugin,
    nativeSmtPluginError,
)
from . import native_assets


class Factory:
    def __init__(self):
        self._map = {}

    def register(self, name, converter_class, connector_class):
        self._map[name] = (converter_class, connector_class)

    def install(self, solver):
        if solver not in self._map:
            raise ValueError(f"unregistered solver {solver}")
        install_python_smt_factory(*self._map[solver])

    def install_native(self, solver):
        if solver not in ("z3", "yices", "cvc5"):
            raise ValueError(f"native backend is unavailable for {solver}")
        distribution = f"maude-se-native-{solver}"
        try:
            version = metadata.version(distribution)
            plugin = importlib.import_module(f"maude_se_native_{solver}")
        except (metadata.PackageNotFoundError, ImportError) as exc:
            raise RuntimeError(
                f"native {solver} plugin is unavailable; run: "
                f"maude-se-installer install native {solver}"
            ) from exc
        core_version = metadata.version("maude-se")
        if version != core_version:
            raise RuntimeError(
                f"{distribution} {version} does not match maude-se {core_version}"
            )
        path = plugin.library_path()
        if not path.is_file():
            raise RuntimeError(f"native {solver} plugin library is missing: {path}")
        if not native_assets.ready(solver, path.parent / "solver"):
            raise RuntimeError(
                f"native {solver} upstream library is missing; run: "
                f"maude-se-installer install native {solver}"
            )
        if not loadNativeSmtPlugin(str(path), solver):
            raise RuntimeError(f"native {solver} plugin could not load: {nativeSmtPluginError()}")
