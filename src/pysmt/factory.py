"""Register Python or separately installed native SMT backends."""

from maudeSE.maude import (
    install_python_smt_factory,
    loadNativeSmtPlugin,
    nativeSmtPluginError,
)
from .plugins import assets as native_assets, plugin as native_plugin


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
        path = native_plugin.library_path(solver)
        if not path.is_file():
            raise RuntimeError(f"native {solver} plugin is unavailable; run: maude-se-installer install native {solver}")
        if not native_assets.ready(solver, path.parent / "solver"):
            raise RuntimeError(
                f"native {solver} upstream library is missing; run: "
                f"maude-se-installer install native {solver}"
            )
        if not loadNativeSmtPlugin(str(path), solver):
            raise RuntimeError(f"native {solver} plugin could not load: {nativeSmtPluginError()}")
