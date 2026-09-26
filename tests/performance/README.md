# Python versus native Maude-SE on PTA2Maude inputs

`compare_pta2maude.py` runs the same model and embedded query through an
installed Python-wheel CLI and a solver-specific standalone executable. It
checks the expected reachability result and rejects warnings, parse errors,
timeouts, and nonzero exits before reporting timings. The models are kept in
`../data/pta2maude/`; this runner does not modify them.

The Python path uses this project's solver connectors and the Z3, Yices, or
cvc5 Python bindings. Despite the source directory name `pysmt`, it does not
use the separate PySMT library.

Example after installing the wheel with Z3 and extracting the Z3 standalone
ZIP (replace the paths with your own):

```sh
python_tag="$(python3 -c 'import sys; print(f"cp{sys.version_info.major}{sys.version_info.minor}")')"
arch="$(uname -m)"
python3 tests/performance/compare_pta2maude.py \
  --python ".build-wheel/${python_tag}-${arch}/venv-test/bin/maude-se" \
  --native "/path/to/maude_se_z3-VERSION-macosx-${arch}/maude-se-z3" \
  --solver z3 --repeats 20 --warmups 2
```

Use `--json` to capture individual runs and metadata. The table shows median
process wall time and the first search time reported by Maude. The latter
excludes some startup work, but it is not a pure SMT-solver timing. Runs use
fresh processes and alternate Python/native order after warmups. Compare only
the same case and solver; a different solver or search algorithm changes the
experiment.

The initial cases are `coffee` and `ex-fig3a` (reachable) plus `ex-fig3b`
(unreachable within its existing 10-step bound). `ex-fig3b` is too small to
serve as a representative throughput benchmark. TGC inputs still contain an
unreplaced `<replace>` goal, while the folding inputs currently report a
`metaSmtCheck` loading error. They are excluded until they can be run without
warnings with identical queries on both paths. The historical
`../data/pta2maude/legacy-test.sh` is not used.
