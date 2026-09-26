# Historical PTA benchmark script

`legacy-test.sh` is retained for reference only. It expects a `models/`
directory that is not present in this repository, checks for the obsolete
`cvc4` executable, and does not correctly apply its solver and timeout
arguments. It must not be treated as a working benchmark or CI test.

The opt-in `../../performance/compare_pta2maude.py` runner compares selected
models through the Python wheel and a solver-specific standalone executable.
See `../../performance/README.md` for supported cases and limitations. It is
not a CI performance gate and does not make `legacy-test.sh` executable.
