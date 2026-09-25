"""Exercise repeated Python/SWIG and Maude GC boundary crossings."""

import gc
import sys
from pathlib import Path

import maudeSE.maude as maude
from maudeSE.factory import Factory
from maudeSE.installer import import_solver


ROOT = Path(__file__).resolve().parents[3]
SOLVERS = {
    "z3": ("maudeSE.converter.z3", "Z3Converter", "maudeSE.connector.z3", "Z3Connector"),
    "yices": ("maudeSE.converter.yices", "YicesConverter", "maudeSE.connector.yices", "YicesConnector"),
    "cvc5": ("maudeSE.converter.cvc5", "Cvc5Converter", "maudeSE.connector.cvc5", "Cvc5Connector"),
}


def run(solver: str, iterations: int = 100) -> None:
    import importlib

    import_solver(solver, solver)
    converter_module, converter_name, connector_module, connector_name = SOLVERS[solver]
    converter = getattr(importlib.import_module(converter_module), converter_name)
    connector = getattr(importlib.import_module(connector_module), connector_name)

    factory = Factory()
    factory.register(solver, converter, connector)
    maude.setSmtSolver(solver)
    factory.install(solver)
    maude.init(advise=False)
    if not maude.load(str(ROOT / "examples/smt-check-ex.maude")):
        raise RuntimeError("failed to load SMT example")
    if not maude.load(str(ROOT / "tests/data/smoke/meta-gcd-python-smoke.maude")):
        raise RuntimeError("failed to load meta SMT search example")
    module = maude.getModule("SMT-CHECK")
    if module is None:
        raise RuntimeError("SMT-CHECK module is unavailable")

    analysis = maude.getModule("GCD-ANALYSIS")
    if analysis is None:
        raise RuntimeError("GCD-ANALYSIS module is unavailable")
    retained = analysis.parseTerm(
        "metaSmtSearch(upModule('GCD, false), upTerm(gcd(10, I)), "
        "upTerm(return(J)), upTerm(I === 6) = upTerm((true).Boolean), "
        "'*, unbounded, 0, 'QF_LRA)"
    )
    if retained is None:
        raise RuntimeError("retained meta SMT search could not be parsed")
    retained.reduce()
    expected_search = str(retained)
    if "'return[" not in expected_search or "'J:Integer <-" not in expected_search:
        raise RuntimeError(f"unexpected retained search result: {expected_search}")

    cases = (
        ("smtCheck(X:Integer > 4)", "(true).Bool"),
        ("smtCheck(X:Integer > 4 and X:Integer < 3)", "(false).Bool"),
        ("smtCheck(X:Integer === 7, true)", "{X:Integer |-> (7).Integer}"),
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
        if str(retained) != expected_search:
            raise RuntimeError("retained meta SMT search result changed after GC churn")

    del retained
    gc.collect()
    final_term = module.parseTerm("smtCheck(X:Integer === 7)")
    if final_term is None:
        raise RuntimeError("SMT term could not be parsed after releasing search result")
    final_term.reduce()
    if str(final_term) != "(true).Bool":
        raise RuntimeError(f"SMT check failed after releasing search result: {final_term}")


if __name__ == "__main__":
    run(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 100)
