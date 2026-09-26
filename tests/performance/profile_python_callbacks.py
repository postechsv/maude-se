#!/usr/bin/env python3
"""Measure Python SMT callback time without changing backend behavior."""

import argparse
import importlib
import os
import time
from collections import defaultdict
from pathlib import Path

from maudeSE.installer import import_solver


ROOT = Path(__file__).resolve().parents[2]
BACKENDS = {
    "z3": ("Z3Converter", "Z3Connector"),
    "yices": ("YicesConverter", "YicesConnector"),
    "cvc5": ("Cvc5Converter", "Cvc5Connector"),
}


def instrument(base, methods, totals):
    overrides = {}
    for name in methods:
        original = getattr(base, name)

        def timed(self, *args, _name=name, _original=original, **kwargs):
            start = time.perf_counter_ns()
            try:
                return _original(self, *args, **kwargs)
            finally:
                record = totals[_name]
                record[0] += 1
                record[1] += time.perf_counter_ns() - start

        overrides[name] = timed
    return type("Profiled" + base.__name__, (base,), overrides)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("solver", choices=BACKENDS)
    parser.add_argument("input", type=Path, help="Maude file with SMT commands")
    args = parser.parse_args()
    model = args.input.resolve()
    if not model.is_file():
        parser.error(f"input does not exist: {model}")

    import_solver(args.solver, args.solver)
    from maudeSE import maude
    from maudeSE.factory import Factory
    import maudeSE

    package_lib = Path(maudeSE.__file__).parent / "maude"
    paths = (model.parent, ROOT / "tests/data/pta2maude", package_lib, ROOT / "src")
    os.environ["MAUDE_LIB"] = os.pathsep.join(map(str, paths)) + os.pathsep + os.environ.get("MAUDE_LIB", "")

    converter_name, connector_name = BACKENDS[args.solver]
    converter = getattr(importlib.import_module(f"maudeSE.converter.{args.solver}"), converter_name)
    connector = getattr(importlib.import_module(f"maudeSE.connector.{args.solver}"), connector_name)
    totals = defaultdict(lambda: [0, 0])
    converter = instrument(converter, ("prepareFor", "dag2term", "term2dag"), totals)
    connector = instrument(connector, ("check_sat", "add_const", "subsume", "get_model"), totals)

    factory = Factory()
    factory.register(args.solver, converter, connector)
    maude.setSmtSolver(args.solver)
    factory.install(args.solver)
    maude.init(advise=False)
    start = time.perf_counter_ns()
    if not maude.load(str(model)):
        raise RuntimeError(f"failed to load {model}")
    elapsed_ms = (time.perf_counter_ns() - start) / 1_000_000
    print(f"\nPython callback profile: {args.solver}, {model}")
    print(f"Maude load and commands: {elapsed_ms:.1f} ms")
    for name, (calls, nanoseconds) in sorted(totals.items()):
        print(f"{name:12s} {calls:6d} calls {nanoseconds / 1_000_000:9.1f} ms")
    print("Callback times include solver work and exclude uninstrumented C++ work.")


if __name__ == "__main__":
    main()
