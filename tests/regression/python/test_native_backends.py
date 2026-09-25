"""Exercise optional native SMT backends inside the Python wheel."""

import gc
import importlib
import sys
from pathlib import Path

import maudeSE.maude as maude
from maudeSE.factory import Factory
from maudeSE.installer import import_solver


ROOT = Path(__file__).resolve().parents[3]


NAMES = {
    "z3": ("Z3Converter", "Z3Connector"),
    "yices": ("YicesConverter", "YicesConnector"),
    "cvc5": ("Cvc5Converter", "Cvc5Connector"),
}


def main(solver: str) -> None:
    import_solver(solver, solver)
    factory = Factory()
    converter_name, connector_name = NAMES[solver]
    converter = getattr(importlib.import_module(f"maudeSE.converter.{solver}"), converter_name)
    connector = getattr(importlib.import_module(f"maudeSE.connector.{solver}"), connector_name)
    factory.register(solver, converter, connector)
    maude.setSmtSolver(solver)
    factory.install_native(solver)
    maude.init(advise=False)
    for path in (ROOT / "examples/smt-check-ex.maude",
                 ROOT / "tests/data/smoke/meta-gcd-python-smoke.maude"):
        if not maude.load(str(path)):
            raise RuntimeError(f"could not load {path}")

    module = maude.getModule("SMT-CHECK")
    cases = {
        "smtCheck(X:Integer > 4)": "(true).Bool",
        "smtCheck(X:Integer > 4 and X:Integer < 3)": "(false).Bool",
        "smtCheck(X:Integer === 7, true)": "{X:Integer |-> (7).Integer}",
    }
    for backend in ("native", "python", "native"):
        if backend == "native":
            factory.install_native(solver)
        else:
            factory.install(solver)
        for expression, expected in cases.items():
            term = module.parseTerm(expression)
            if term is None:
                raise RuntimeError(f"could not parse {expression}")
            term.reduce()
            if str(term) != expected:
                raise RuntimeError(f"{backend}: {expression} -> {term}")
            del term
        simplified = module.parseTerm("simplifyFormula(X:Integer + 0)")
        simplified.reduce()
        if str(simplified) not in ("X:Integer", "X:Integer + (0).Integer"):
            raise RuntimeError(f"{backend}: unexpected simplification: {simplified}")
        del simplified
        gc.collect()

    analysis = maude.getModule("GCD-ANALYSIS")
    query = analysis.parseTerm(
        "metaSmtSearch(upModule('GCD, false), upTerm(gcd(10, I)), "
        "upTerm(return(J)), upTerm(I === 6) = upTerm((true).Boolean), "
        "'*, unbounded, 0, 'QF_LRA)"
    )
    if query is None:
        raise RuntimeError("could not parse native SMT search")
    query.reduce()
    if "'return[" not in str(query) or "'J:Integer <-" not in str(query):
        raise RuntimeError(f"unexpected native SMT search result: {query}")


if __name__ == "__main__":
    main(sys.argv[1])
