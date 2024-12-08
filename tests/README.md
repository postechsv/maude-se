# Test files for maude-se

We give some test maude files for maude-se tool.
the test files give a rewriting logic semantics for parametric time automata (PTAs)
and show that symbolic reachablity analysis using Maude-se.  


### Maude files

The specification can be found in the folder `tests`. It includes the following files:

- _pta-base_: Sorts for defining locations, states and constrained terms.
- _meta-pta_: Reachability procedures with folding.
- _coffee.maude_: Example of the coffee machine using the command smt-search.