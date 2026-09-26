# Historical PTA benchmark script

`legacy-test.sh` is retained for reference only. It expects a `models/`
directory that is not present in this repository, checks for the obsolete
`cvc4` executable, and does not correctly apply its solver and timeout
arguments. It must not be treated as a working benchmark or CI test.

There is currently no Maude-SE performance benchmark in this repository. A
future benchmark should specify the exact model and query, solver, Python or
standalone execution path, repetition count, timeout, expected result, and
environment before reporting timings.
