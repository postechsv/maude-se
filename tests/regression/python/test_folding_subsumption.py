"""Folding must account for SMT constraints and preserve SWIG term ownership."""

import gc
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
        ROOT / "tests/data/smoke/folding-subsumption.maude",
        ROOT / "tests/data/smoke/meta-gcd-python-smoke.maude",
    ):
        if not maude.load(str(path)):
            raise RuntimeError(f"failed to load {path}")

    module = maude.getModule("FOLDING-SUBSUMPTION-ANALYSIS")
    for fold in ("false", "true"):
        for target in (1, 2):
            expression = (
                "metaSmtSearch(upModule('FOLDING-SUBSUMPTION, false), "
                f"upTerm(source(I)), upTerm(branch({target})), "
                "upTerm((true).Boolean) = upTerm((true).Boolean), "
                f"'*, 2, 0, 'QF_LRA, {fold})"
            )
            term = module.parseTerm(expression)
            if term is None:
                raise RuntimeError(f"could not parse {expression}")
            term.reduce()
            result = str(term)
            if target == 1 and ("'branch[" not in result or "'I:Integer <- '1.Integer" not in result):
                raise RuntimeError(f"fold={fold} discarded the satisfiable branch: {result}")
            if target == 2 and "failure" not in result:
                raise RuntimeError(f"fold={fold} reported an unreachable branch: {result}")

    # Exercise more than one candidate match and Python-owned key wrappers.
    gcd = maude.getModule("GCD-ANALYSIS")
    expression = (
        "metaSmtSearch(upModule('GCD, false), upTerm(gcd(10, I)), "
        "upTerm(return(J)), upTerm(I === 6) = upTerm((true).Boolean), "
        "'*, unbounded, 0, 'QF_LRA, true)"
    )
    for _ in range(10):
        term = gcd.parseTerm(expression)
        if term is None:
            raise RuntimeError("could not parse folding GCD search")
        term.reduce()
        if "'return[" not in str(term):
            raise RuntimeError(f"folding GCD search failed: {term}")
        del term
        gc.collect()


if __name__ == "__main__":
    run(sys.argv[1])
