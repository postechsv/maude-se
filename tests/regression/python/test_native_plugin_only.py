"""Check native plugins without importing any Python solver package."""

import gc
import sys
from pathlib import Path

import maudeSE.maude as maude
from maudeSE.factory import Factory


ROOT = Path(__file__).resolve().parents[3]


def main(solver: str) -> None:
    maude.setSmtSolver(solver)
    Factory().install_native(solver)
    maude.init(advise=False)
    for path in (ROOT / "examples/smt-check-ex.maude",
                 ROOT / "tests/data/smoke/meta-gcd-python-smoke.maude"):
        if not maude.load(str(path)):
            raise RuntimeError(f"could not load {path}")

    module = maude.getModule("SMT-CHECK")
    for _ in range(30):
        for expression, expected in (
            ("smtCheck(X:Integer > 4)", "(true).Bool"),
            ("smtCheck(X:Integer > 4 and X:Integer < 3)", "(false).Bool"),
            ("smtCheck(X:Integer === 7, true)", "{X:Integer |-> (7).Integer}"),
        ):
            term = module.parseTerm(expression)
            term.reduce()
            if str(term) != expected:
                raise RuntimeError(f"{solver}: {expression} -> {term}")
            del term
        gc.collect()

    analysis = maude.getModule("GCD-ANALYSIS")
    search = analysis.parseTerm(
        "metaSmtSearch(upModule('GCD, false), upTerm(gcd(10, I)), "
        "upTerm(return(J)), upTerm(I === 6) = upTerm((true).Boolean), "
        "'*, unbounded, 0, 'QF_LRA)"
    )
    search.reduce()
    if "'return[" not in str(search) or "'J:Integer <-" not in str(search):
        raise RuntimeError(f"unexpected {solver} search result: {search}")


if __name__ == "__main__":
    main(sys.argv[1])
