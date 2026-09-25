"""Exercise repeated Python/SWIG and Maude GC boundary crossings."""

import gc
import sys
from pathlib import Path

import maudeSE.maude as maude
from maudeSE.factory import Factory


ROOT = Path(__file__).resolve().parents[1]
SOLVERS = {
    "z3": ("maudeSE.converter.z3", "Z3Converter", "maudeSE.connector.z3", "Z3Connector"),
    "yices": ("maudeSE.converter.yices", "YicesConverter", "maudeSE.connector.yices", "YicesConnector"),
    "cvc5": ("maudeSE.converter.cvc5", "Cvc5Converter", "maudeSE.connector.cvc5", "Cvc5Connector"),
}


def run(solver: str, iterations: int = 100) -> None:
    import importlib

    converter_module, converter_name, connector_module, connector_name = SOLVERS[solver]
    converter = getattr(importlib.import_module(converter_module), converter_name)
    connector = getattr(importlib.import_module(connector_module), connector_name)

    factory = Factory().__disown__()
    factory.register(solver, converter, connector)
    maude.setSmtSolver(solver)
    maude.setSmtManagerFactory(factory)
    maude.init(advise=False)
    if not maude.load(str(ROOT / "examples/smt-check-ex.maude")):
        raise RuntimeError("failed to load SMT example")
    module = maude.getModule("SMT-CHECK")
    if module is None:
        raise RuntimeError("SMT-CHECK module is unavailable")

    cases = (
        ("smtCheck(X:Integer > 4)", "(true).Bool"),
        ("smtCheck(X:Integer > 4 and X:Integer < 3)", "(false).Bool"),
    )
    for _ in range(iterations):
        for expression, expected in cases:
            term = module.parseTerm(expression)
            if term is None:
                raise RuntimeError(f"SMT term could not be parsed: {expression}")
            term.reduce()
            if str(term) != expected:
                raise RuntimeError(f"unexpected SMT result: {term}")
            del term
        gc.collect()


if __name__ == "__main__":
    run(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 100)
