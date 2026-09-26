"""Ensure the paper's Python extension points remain active across the C++ bridge."""

from pathlib import Path

import maudeSE.maude as maude
from maudeSE.connector.z3 import Z3Connector
from maudeSE.converter.z3 import Z3Converter
from maudeSE.factory import Factory


ROOT = Path(__file__).resolve().parents[3]


class CustomConverter(Z3Converter):
    conversions = 0
    reverse_conversions = 0

    def dag2term(self, term):
        type(self).conversions += 1
        return super().dag2term(term)

    def term2dag(self, term):
        type(self).reverse_conversions += 1
        return super().term2dag(term)


class CustomConnector(Z3Connector):
    checks = 0
    models = 0
    accumulated = 0
    force_unsat = False

    def check_sat(self, constraints):
        type(self).checks += 1
        if type(self).force_unsat:
            return maude.unsat
        return super().check_sat(constraints)

    def get_model(self):
        type(self).models += 1
        return super().get_model()

    def add_const(self, accumulated, current):
        type(self).accumulated += 1
        return super().add_const(accumulated, current)


def reduce_check(module, expression):
    term = module.parseTerm(expression)
    if term is None:
        raise RuntimeError(f"could not parse {expression}")
    term.reduce()
    return str(term)


def run():
    factory = Factory()
    factory.register("z3", CustomConverter, CustomConnector)
    maude.setSmtSolver("z3")
    factory.install("z3")
    maude.init(advise=False)
    if not maude.load(str(ROOT / "examples/smt-check-ex.maude")):
        raise RuntimeError("failed to load SMT example")
    module = maude.getModule("SMT-CHECK")

    if reduce_check(module, "smtCheck(X:Integer > 4)") != "(true).Bool":
        raise RuntimeError("custom connector changed the default SAT result")
    if reduce_check(module, "smtCheck(X:Integer === 7, true)") != "{X:Integer |-> (7).Integer}":
        raise RuntimeError("custom connector changed model generation")
    if (CustomConverter.conversions < 2 or CustomConverter.reverse_conversions < 1
            or CustomConnector.checks < 2 or CustomConnector.models < 1):
        raise RuntimeError("Python converter or connector callback was bypassed")

    # A user-defined connector must be able to change solving without rebuilding Maude.
    CustomConnector.force_unsat = True
    try:
        if reduce_check(module, "smtCheck(X:Integer > 4)") != "(false).Bool":
            raise RuntimeError("custom Python satisfiability policy was ignored")
    finally:
        CustomConnector.force_unsat = False

    if not maude.load(str(ROOT / "tests/data/smoke/meta-gcd-python-smoke.maude")):
        raise RuntimeError("failed to load meta SMT search example")
    analysis = maude.getModule("GCD-ANALYSIS")
    expression = (
        "metaSmtSearch(upModule('GCD, false), upTerm(gcd(10, I)), "
        "upTerm(return(J)), upTerm(I === 6) = upTerm((true).Boolean), "
        "'*, unbounded, 0, 'QF_LRA)"
    )
    result = analysis.parseTerm(expression)
    if result is None:
        raise RuntimeError("failed to parse meta SMT search")
    result.reduce()
    if "'return[" not in str(result):
        raise RuntimeError("unexpected meta SMT search result")
    if CustomConnector.accumulated < 1:
        raise RuntimeError("custom Python constraint accumulation was bypassed")


if __name__ == "__main__":
    run()
