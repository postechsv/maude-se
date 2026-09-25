"""Register Python SMT backends with the C++-owned factory."""

from maudeSE.maude import install_python_smt_factory


class Factory:
    def __init__(self):
        self._map = {}

    def register(self, name, converter_class, connector_class):
        self._map[name] = (converter_class, connector_class)

    def install(self, solver):
        if solver not in self._map:
            raise ValueError(f"unregistered solver {solver}")
        install_python_smt_factory(*self._map[solver])
