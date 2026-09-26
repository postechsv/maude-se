# Tests

`regression/` contains executable checks. `data/` contains their inputs and
other retained models. Public Maude-SE paper and manual examples live in the
top-level `examples/` directory.

| Directory | Contents |
| --- | --- |
| `regression/python/` | Python/SWIG unit and solver integration checks |
| `regression/native/` | Standalone executable checks |
| `performance/` | Opt-in Python-versus-native comparison; see its README for limitations |
| `data/smoke/` | Small regression inputs |
| `data/general/` | General Maude models, including Bakery and Dining Philosophers |
| `data/maude-se-2020/` | GCD and robot examples from the 2020 Maude-SE paper |
| `data/pta2maude/` | Adapted PTA2Maude research models and historical script |

From the repository root, build and install the wheel and solver extras into
the test environment with `./build.sh wheel` and `./build.sh test-wheel`.
Open that environment with `./build.sh shell`, then run the unit checks and
solver integration checks below. The `python` command must be from that shell,
not an unrelated system installation.

```sh
python -m unittest discover -s tests/regression/python -p 'test_*.py'
```

Integration scripts are not run by unittest discovery. Most take a solver
name; the Z3-specific checks run without an argument:

```sh
for solver in z3 yices cvc5; do
  python tests/regression/python/test_backend_lifetime.py "$solver"
  python tests/regression/python/test_gc_lifetime.py "$solver"
  python tests/regression/python/test_python_meta_search.py "$solver"
  python tests/regression/python/test_folding_subsumption.py "$solver"
done
python tests/regression/python/test_custom_connector_interface.py
python tests/regression/python/test_managed_z3_converter.py
for solver in yices cvc5; do
  python tests/regression/python/test_managed_other_converters.py "$solver"
done
```

Native plugin checks additionally require running
`maude-se-installer install native all --source-dir .` in that shell. Then run
both native scripts for each solver:

```sh
for solver in z3 yices cvc5; do
  python tests/regression/python/test_native_plugin_only.py "$solver"
  python tests/regression/python/test_native_backends.py "$solver"
done
```

After building a standalone executable, run the Maude-SE feature checks and
general-model reachability checks with any solver variant. The cvc5-specific
Real-model check requires the cvc5 executable:

```sh
tests/regression/native/test_maude_se_features.sh /path/to/maude-se-cvc5
tests/regression/native/test_general_models.sh /path/to/maude-se-cvc5
tests/regression/native/test_native_cvc5_real_model.sh /path/to/maude-se-cvc5
```

The Maude-SE feature check covers SAT/UNSAT, a concrete SMT model, both
reachable and unreachable `smt-search` GCD goals, and the 2020 Core Maude
GCD example using `smtCheck`. The Python GC check retains a meta-search
result through repeated SMT calls and Python collections, then checks that
SMT still works after releasing it.

The general-model check searches for a Bakery critical-section state and a
Dining Philosophers eating state. It does not establish mutual exclusion,
deadlock freedom, or fairness. Files in `data/` without a regression check
are retained inputs, not automatically passing tests.
