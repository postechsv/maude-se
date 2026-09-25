"""Ensure completed SMT checks release their Python and native backends."""

import gc
import importlib
import sys
import weakref
from pathlib import Path

import maudeSE.maude as maude
from maudeSE.factory import Factory
from maudeSE.installer import import_solver


ROOT = Path(__file__).resolve().parents[3]
BACKENDS = {
    "z3": ("Z3Converter", "Z3Connector"),
    "yices": ("YicesConverter", "YicesConnector"),
    "cvc5": ("Cvc5Converter", "Cvc5Connector"),
}


def run(solver: str, iterations: int = 20) -> None:
    import_solver(solver, solver)
    converter_name, connector_name = BACKENDS[solver]
    converter = getattr(importlib.import_module(f"maudeSE.converter.{solver}"), converter_name)
    connector = getattr(importlib.import_module(f"maudeSE.connector.{solver}"), connector_name)
    references = []

    class TrackedConverter(converter):
        def __init__(self):
            super().__init__()
            references.append(weakref.ref(self))

    factory = Factory()
    factory.register(solver, TrackedConverter, connector)
    factory.install(solver)
    maude.setSmtSolver(solver)
    maude.init(advise=False)
    if not maude.load(str(ROOT / "examples/smt-check-ex.maude")):
        raise RuntimeError("failed to load SMT example")
    module = maude.getModule("SMT-CHECK")

    if solver == "yices":
        from yices import Config, Context
        initial_population = (Context.population(), Config.population())

    for iteration in range(iterations):
        term = module.parseTerm("smtCheck(X:Integer > 4)")
        if term is None:
            raise RuntimeError("SMT check could not be parsed")
        term.reduce()
        if str(term) != "(true).Bool":
            raise RuntimeError(f"unexpected SMT result: {term}")
        del term
        gc.collect()
        if any(reference() is not None for reference in references):
            raise RuntimeError(f"converter survived completed SMT check {iteration + 1}")
        if solver == "yices" and (Context.population(), Config.population()) != initial_population:
            raise RuntimeError(f"Yices native resources survived check {iteration + 1}")


if __name__ == "__main__":
    run(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 20)
