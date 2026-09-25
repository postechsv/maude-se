"""Regression check for Python solver AST equality during meta SMT search."""

from pathlib import Path

import maudeSE.maude as maude
from maudeSE.connector.z3 import Z3Connector
from maudeSE.converter.z3 import Z3Converter
from maudeSE.factory import Factory


ROOT = Path(__file__).resolve().parents[1]
def run() -> None:
    factory = Factory().__disown__()
    factory.register("z3", Z3Converter, Z3Connector)
    maude.setSmtSolver("z3")
    maude.setSmtManagerFactory(factory)
    maude.init(advise=False)
    for path in (
        ROOT / "examples/smt-check-ex.maude",
        ROOT / "tests/examples/meta-gcd-python-smoke.maude",
    ):
        if not maude.load(str(path)):
            raise RuntimeError(f"failed to load {path}")

    module = maude.getModule("GCD-ANALYSIS")
    expression = (
        "metaSmtSearch(upModule('GCD, false), upTerm(gcd(10, I)), "
        "upTerm(return(J)), upTerm(I < 9 and I > 0) = "
        "upTerm((true).Boolean), '*, unbounded, 0, 'QF_LRA)"
    )
    term = module.parseTerm(expression)
    if term is None:
        raise RuntimeError("meta SMT search could not be parsed")
    term.reduce()
    result = str(term)
    if "'return[" not in result or "'J:Integer <-" not in result:
        raise RuntimeError(f"unexpected meta SMT search result: {term}")


if __name__ == "__main__":
    run()
