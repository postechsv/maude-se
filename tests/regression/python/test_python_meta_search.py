"""Regression checks for Python solvers during meta SMT search."""

import importlib
import sys
from pathlib import Path

import maudeSE.maude as maude
from maudeSE.factory import Factory
from maudeSE.installer import import_solver


ROOT = Path(__file__).resolve().parents[3]
SOLVERS = {
    "z3": ("maudeSE.converter.z3", "Z3Converter", "maudeSE.connector.z3", "Z3Connector"),
    "cvc5": ("maudeSE.converter.cvc5", "Cvc5Converter", "maudeSE.connector.cvc5", "Cvc5Connector"),
    "yices": ("maudeSE.converter.yices", "YicesConverter", "maudeSE.connector.yices", "YicesConnector"),
}


def run(solver: str) -> None:
    import_solver(solver, solver)
    converter_module, converter_name, connector_module, connector_name = SOLVERS[solver]
    converter = getattr(importlib.import_module(converter_module), converter_name)
    connector = getattr(importlib.import_module(connector_module), connector_name)

    factory = Factory()
    factory.register(solver, converter, connector)
    maude.setSmtSolver(solver)
    factory.install(solver)
    maude.init(advise=False)
    for path in (
        ROOT / "examples/smt-check-ex.maude",
        ROOT / "tests/data/smoke/meta-gcd-python-smoke.maude",
    ):
        if not maude.load(str(path)):
            raise RuntimeError(f"failed to load {path}")

    module = maude.getModule("GCD-ANALYSIS")
    expression = (
        "metaSmtSearch(upModule('GCD, false), upTerm(gcd(10, I)), "
        "upTerm(return(J)), upTerm(I < 9 and I > 0) = "
        "upTerm((true).Boolean), '*, unbounded, 0, 'QF_LRA)"
    )
    expected = None
    for _ in range(20):
        term = module.parseTerm(expression)
        if term is None:
            raise RuntimeError("meta SMT search could not be parsed")
        term.reduce()
        result = str(term)
        if "'return[" not in result or "'J:Integer <-" not in result:
            raise RuntimeError(f"unexpected meta SMT search result: {term}")
        if expected is None:
            expected = result
        elif result != expected:
            raise RuntimeError("repeated meta SMT search changed its result")


if __name__ == "__main__":
    run(sys.argv[1])
