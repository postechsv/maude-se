"""Exercise the term-level Maude-SE SMT search API and its result lifetime."""

import gc
import importlib
import sys
from pathlib import Path

import maudeSE.maude as maude
from maudeSE.factory import Factory
from maudeSE.installer import import_solver


ROOT = Path(__file__).resolve().parents[3]
SOLVERS = {
    "z3": ("Z3Converter", "Z3Connector"),
    "cvc5": ("Cvc5Converter", "Cvc5Connector"),
    "yices": ("YicesConverter", "YicesConnector"),
}


def run(solver: str) -> None:
    import_solver(solver, solver)
    converter_name, connector_name = SOLVERS[solver]
    converter = getattr(importlib.import_module(f"maudeSE.converter.{solver}"), converter_name)
    connector = getattr(importlib.import_module(f"maudeSE.connector.{solver}"), connector_name)
    factory = Factory()
    factory.register(solver, converter, connector)
    maude.setSmtSolver(solver)
    factory.install(solver)
    maude.init(advise=False)
    assert maude.load(str(ROOT / "examples/smt-check-ex.maude"))
    assert maude.load(str(ROOT / "tests/data/smoke/meta-gcd-python-smoke.maude"))

    module = maude.getModule("GCD-ANALYSIS")
    initial = module.parseTerm("gcd(10, I)")
    target = module.parseTerm("gcd(X:Integer, Y:Integer)")
    true_goal = module.parseTerm("(true).Boolean")
    false_goal = module.parseTerm("(false).Boolean")
    assert all(term is not None for term in (initial, target, true_goal, false_goal))

    search = initial.smtSearch(maude.ANY_STEPS, target, true_goal, fold=False)
    assert search is not None
    state = next(search)
    constraint = search.getFinalConstraint()
    substitution = search.getSubstitution()
    assert str(state) == str(initial)
    assert "X:Integer" in str(constraint)
    assert "Y:Integer" in str(substitution)
    del search
    gc.collect()
    assert str(state) == str(initial)
    assert "X:Integer" in str(constraint)
    assert "Y:Integer" in str(substitution)

    no_solution = initial.smtSearch(maude.ANY_STEPS, target, false_goal, fold=False)
    assert list(no_solution) == []
    assert not no_solution.isSmtUnknown()
    assert not no_solution.hasInvalidRewriteResult()


if __name__ == "__main__":
    run(sys.argv[1])
