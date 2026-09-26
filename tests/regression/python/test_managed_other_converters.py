"""Exercise managed Yices/cvc5 converters through the Maude Python bridge."""

import importlib
from pathlib import Path
import sys

import maudeSE.maude as maude
from maudeSE.factory import Factory
from maudeSE.installer import import_solver


ROOT = Path(__file__).resolve().parents[3]
NAMES = {"yices": "YicesConverter", "cvc5": "Cvc5Converter"}


def run(solver):
    import_solver(solver, solver)
    converter = getattr(importlib.import_module(f"maudeSE.converter.{solver}"), NAMES[solver])
    connector = getattr(
        importlib.import_module(f"maudeSE.connector.{solver}"),
        "YicesConnector" if solver == "yices" else "Cvc5Connector",
    )
    if not hasattr(converter, "__wrapped__"):
        raise RuntimeError(f"{solver} converter is not managed")

    factory = Factory()
    factory.register(solver, converter, connector)
    maude.setSmtSolver(solver)
    factory.install(solver)
    maude.init(advise=False)
    if not maude.load(str(ROOT / "examples/smt-check-ex.maude")):
        raise RuntimeError("failed to load SMT example")
    module = maude.getModule("SMT-CHECK")
    for expression, expected in (
        ("smtCheck(X:Integer > 4)", "(true).Bool"),
        ("smtCheck(X:Integer === 7, true)", "{X:Integer |-> (7).Integer}"),
    ):
        term = module.parseTerm(expression)
        term.reduce()
        if str(term) != expected:
            raise RuntimeError(f"{solver}: {expression} -> {term}")

    if not maude.load(str(ROOT / "tests/data/smoke/smt-var-id-managed.maude")):
        raise RuntimeError("failed to load SMTVarId example")
    var_module = maude.getModule("SMT-VAR-ID-MANAGED")
    source = var_module.parseTerm("i(a)")
    instance = converter()
    instance.prepareFor(var_module)
    restored = instance.term2dag(instance.dag2term(source))
    if str(restored) != "i(a)":
        raise RuntimeError(f"{solver}: SMTVarId round-trip changed: {restored}")


if __name__ == "__main__":
    run(sys.argv[1])
