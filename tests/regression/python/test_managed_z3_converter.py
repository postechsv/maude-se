"""Exercise the managed Z3 converter through Maude's actual Python bridge."""

from pathlib import Path

import maudeSE.maude as maude
from maudeSE.connector.z3 import Z3Connector
from maudeSE.converter.z3 import Z3Converter
from maudeSE.factory import Factory


ROOT = Path(__file__).resolve().parents[3]


def run():
    if not hasattr(Z3Converter, "__wrapped__"):
        raise RuntimeError("Z3Converter is not using the managed converter")

    factory = Factory()
    factory.register("z3", Z3Converter, Z3Connector)
    maude.setSmtSolver("z3")
    factory.install("z3")
    maude.init(advise=False)
    if not maude.load(str(ROOT / "examples/smt-check-ex.maude")):
        raise RuntimeError("failed to load SMT example")
    module = maude.getModule("SMT-CHECK")
    for expression, expected in (
        ("smtCheck(X:Integer > 4)", "(true).Bool"),
        ("smtCheck(X:Integer === 7, true)", "{X:Integer |-> (7).Integer}"),
    ):
        term = module.parseTerm(expression)
        if term is None:
            raise RuntimeError(f"failed to parse {expression}")
        term.reduce()
        if str(term) != expected:
            raise RuntimeError(f"{expression}: expected {expected}, got {term}")

    # 2020 Maude-SE ground SMT variables use b/i/r(SMTVarId), not Maude
    # variables.  Their original DAG must survive a solver round-trip.
    if not maude.load(str(ROOT / "tests/data/smoke/smt-var-id-managed.maude")):
        raise RuntimeError("failed to load SMTVarId example")
    var_module = maude.getModule("SMT-VAR-ID-MANAGED")
    source = var_module.parseTerm("i(a)")
    converter = Z3Converter()
    converter.prepareFor(var_module)
    restored = converter.term2dag(converter.dag2term(source))
    if str(restored) != "i(a)":
        raise RuntimeError(f"SMTVarId variable changed during round-trip: {restored}")


if __name__ == "__main__":
    run()
