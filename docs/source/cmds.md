# Analysis Commands

MaudeSE provides various analysis commands: 

* {ref}`check` 
* {ref}`show-model` 
* {ref}`smt-search` 
* {ref}`show-smt-path`

(check)=
## check

This command determines the satisfiability of Boolean formulas. Given a module `M`, a Boolean formula `f`, and an SMT theory `Th`,
it checks whether `f` is satisfiable under `Th`.

```maude
MaudeSE> check in M : f using Th .
```

For example, consider the module `EUF-EX` ({download}`euf-ex.maude <assets/euf-ex.maude>`) which contains uninterpreted function symbols 
declared using `metadata` with `smt euf`.

```maude
fmod EUF-EX is
  protecting INTEGER .
  sort A .

  op f : A -> A [metadata "smt euf"] .
  op _xor_ : Integer Integer -> A [prec 30 metadata "smt euf"] .
  op _===_ : A A -> Boolean .
endfm
```

```{note}
See {doc}`examples` to learn how to declare SMT symbols using `metadata`.
```

You can check satisfiability using the following command:

```maude
MaudeSE> check in EUF-EX : I:Integer > 2 and 
            f(X:A) === (I:Integer xor J:Integer) using QF_UFLIA .
result: sat
```

(show-model)=
## show model

The show model command returns the satisfying assignment, if any, for the most recent `check` command.
For example, you can obtain the assignment for the above check example as follows:

```maude
MaudeSE> show model .

  assignment:
    I:Integer |--> 3
    J:Integer |--> 2
```

(smt-search)=
## smt-search

This command performs symbolic reachability analysis with folding. Given an initial term `t`, a goal pattern `u`, and a goal condition `c`, it searches for the `n`-th solution that is reachable from `t` within `m` rewrite steps, matches the goal pattern `u`, and satisfies the condition `c` under the SMT theory `Th`:

```maude
MaudeSE> {fold} smt-search [n,m] in M : t =>* u such that c using Th .
```

The arguments `{fold}`, `n`, and `m` are optional. If `{fold}` is specified, the command ignores constrained terms that are subsumed by others.

As an example, consider the coffee machine module, introduced in {doc}`getting_started`.
The following command finds the first solution that reaches the `cdone` state, where all clocks and parameters are greater than or equal to 0.

```maude
MaudeSE> {fold} smt-search [1] in COFFEE-MACHINE : 
            < idle ; X:Real ; Y:Real > < P1:Real ; P2:Real ; P3:Real > =>* 
            < cdone ; X':Real ; Y':Real > < P1:Real ; P2:Real ; P3:Real > 
            such that X:Real === Y:Real and X:Real >= 0/1 and Y:Real >= 0/1 and 
                      P1:Real >= 0/1 and P2:Real >= 0/1 and P3:Real >= 0/1 using QF_LRA .

Solution 1 (state 12)

Symbolic state:
 < cdone ; #58-V35:Real ; #59-V36:Real > < #60-V37:Real ; #61-V38:Real ; #62-V39:Real >

Constraint:
 not (not V0:Real === X:Real or ...)

Substitution:
 V5:Real <-- #58-V35:Real
 V6:Real <-- #59-V36:Real
 V7:Real <-- #60-V37:Real
 V8:Real <-- #61-V38:Real
 V9:Real <-- #62-V39:Real

Assignment:
 #1-X:Real <-- 0/1
 #10-P3:Real <-- 0/1
 ...

Concrete state:
 < cdone ; 10/1 ; 10/1 > < 0/1 ; 0/1 ; 0/1 >
```

(show-smt-path)=
## show smt-path

This command returns either a symbolic or concrete path for the most recent `smt-search` result. It takes a path type as an argument (symbolic or concrete). 

A symbolic path consists of a sequence of contracted terms and rewrite rules. The corresponding concrete path is an instance of the symbolic path instantiated with a satisfying assignment. For example, the following show the symbolic and concrete paths of the above search command.

```maude
MaudeSE> show smt-path concrete .
< idle ; 0/1 ; 0/1 > < 0/1 ; 0/1 ; 0/1 >
=====[toAddsugar]=====>
[addsugar ; 0/1 ; 0/1]< 0/1 ; 0/1 ; 0/1 >
 ...

MaudeSE> show smt-path symbolic .
Constrained Term:
 Term: < idle ; V0:Real ; V1:Real > < V2:Real ; V3:Real ; V4:Real >
 Constraint: not (not V0:Real === X:Real or ...)
=====[toAddsugar]=====>
Constrained Term:
 Term: [addsugar ; #5-V15:Real ; #6-V16:Real]< #7-V17:Real ; #8-V18:Real ; #9-V19:Real >
 Constraint: not (not V0:Real === X:Real or ...)
 ...
```