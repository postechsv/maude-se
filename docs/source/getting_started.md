# Getting started

## Example Model

As an example model, consider the specification of coffee machine ({download}`coffee.maude <assets/coffee.maude>`) adapted from {cite}`arias2022pta`. The system module `COFFEE-MACHINE` specifies the coffee machine parametric timed automaton (PTA). Its behavior is the same as in the original version {cite}`arias2022pta`, except that we use auxiliary functions and equations to separate each rule condition into a non-SMT condition and a pure SMT formula.

```maude
mod COFFEE-MACHINE is
  protecting REAL .

  sorts State Location .

  vars T X Y X' Y' P1 P2 P3 : Real .
  vars L L' : Location .
  var PHI : Boolean .

  ops idle addsugar preparingcoffee cdone : -> Location .
  op <_;_;_> <_;_;_> : Location Real Real Real Real Real -> State .
  op [_;_;_] <_;_;_> : Location Real Real Real Real Real -> State .

  sort Tuple{Location,Real,Real,Boolean} .
  op {_,_,_,_} : Location Real Real Boolean -> Tuple{Location,Real,Real,Boolean} [ctor] .

  crl [tick] : [ L ; X ; Y ] < P1 ; P2 ; P3 >
   => < L ; X + T ; Y + T > < P1 ; P2 ; P3 >
  if PHI := tickCond(L, T, X, Y, P2, P3) /\ (T >= 0/1 and PHI) = true [nonexec] .

  op tickCond : Location Real Real Real Real Real -> Boolean .
  eq tickCond(idle, T, X, Y, P2, P3) = true .
  eq tickCond(addsugar, T, X, Y, P2, P3) = Y + T <= P2 .
  eq tickCond(preparingcoffee, T, X, Y, P2, P3) = Y + T <= P3 .
  eq tickCond(cdone, T, X, Y, P2, P3) = X + T <= 10/1 .

  crl [toAddsugar] : < L ; X ; Y > < P1 ; P2 ; P3 >
   => [ L' ; X' ; Y' ] < P1 ; P2 ; P3 >
  if {L', X', Y', PHI} := nextAddSugar(L, X, Y, P1, P2) /\ PHI = true .

  op nextAddSugar : Location Real Real Real Real -> Tuple{Location,Real,Real,Boolean} .
  eq nextAddSugar(idle, X, Y, P1, P2) = {addsugar, 0/1, 0/1, 0/1 <= P2} .
  eq nextAddSugar(addsugar, X, Y, P1, P2) = {addsugar, 0/1, Y, Y <= P2 and X >= P1} .
  eq nextAddSugar(cdone, X, Y, P1, P2) = {addsugar, 0/1, 0/1, 0/1 <= P2} .

  crl [toOther] : < L ; X ; Y > < P1 ; P2 ; P3 >
  => [ L' ; X' ; Y' ] < P1 ; P2 ; P3 >
  if {L', X', Y', PHI} := nextOther(L, X, Y, P2, P3) /\ PHI = true .

  op nextOther : Location Real Real Real Real -> Tuple{Location,Real,Real,Boolean} .
  eq nextOther(addsugar, X, Y, P2, P3) = {preparingcoffee, X, Y, Y <= P3 and Y === P2} .
  eq nextOther(preparingcoffee, X, Y, P2, P3) = {cdone, 0/1, Y, 0/1 <= 10/1 and Y === P3} .
  eq nextOther(cdone, X, Y, P2, P3) = {idle, X, Y, X === 10/1} .
endm
```

---

## Symbolic Reachability Analysis

### Basic Usage

Use the following commmand to load the example Maude file and set the SMT solver to Yices2.

```console
$ maude-se coffee.maude -s yices
```

```{note}
Refer to the section below ({ref}`standalone-usage-notes`) to learn how to use the MaudeSE standalone executable, which differs slightly.
```

If successful, the MaudeSE interpreter will appear as follows.

```maude
          ===================================
                        MaudeSE
               (0.0.3 built: June 6 2025)
          ===================================


MaudeSE>
```

Load the COFFEE-MACHINE module with `select`.

```maude
MaudeSE> select COFFEE-MACHINE .
Successfully load module.
```

Use the `smt-search` command to perform symbolic reachability analysis.
For example, you can find the first solution where the coffee machine is in the `cdone` state
and `P1`, `P2`, and `P3` are all positive real numbers,
starting from the `idle` state with a positive real number `X1` equal to `X2`.

```maude
MaudeSE> smt-search [1] [ idle ; X1:Real ; X2:Real ] < P1:Real ; P2:Real ; P3:Real > 
                 =>* < cdone ; X1':Real ; X2':Real > < P1:Real ; P2:Real ; P3:Real >
      such that X1:Real === X2:Real and X1:Real >= 0/1 and 
                P1:Real >= 0/1 and P2:Real >= 0/1 and P3:Real >= 0/1 using QF_LRA .
```

The command returns the first solution as follows.

```maude
Solution 1 (state 13)

Symbolic state:
 < cdone ; #69-V35:Real ; #70-V36:Real > < #71-V37:Real ; #72-V38:Real ; #73-V39:Real >

Constraint:
 not not V0:Real === X1:Real or ... #71-V37:Real ...

Substitution:
 V5:Real <-- #69-V35:Real
 V6:Real <-- #70-V36:Real
 V7:Real <-- #71-V37:Real
 V8:Real <-- #72-V38:Real
 V9:Real <-- #73-V39:Real

Assignment:
 #1-T:Real <-- 0/1
 #10-V39:Real <-- 0/1
 #11-P1:Real <-- 0/1
 ...
 P1:Real <-- 0/1
 P2:Real <-- 0/1
 P3:Real <-- 0/1
 X1':Real <-- 10/1
 X1:Real <-- 0/1
 X2':Real <-- 10/1
 X2:Real <-- 0/1

Concrete state:
 < cdone ; 10/1 ; 10/1 > < 0/1 ; 0/1 ; 0/1 >
```

To display the concrete path of the analysis, use the `show smt-path` command.

```maude
MaudeSE> show smt-path concrete .

[idle ; 0/1 ; 0/1]< 0/1 ; 0/1 ; 0/1 >
=====[tick]=====>
< idle ; 0/1 ; 0/1 > < 0/1 ; 0/1 ; 0/1 >
=====[toAddsugar]=====>
[addsugar ; 0/1 ; 0/1]< 0/1 ; 0/1 ; 0/1 >
=====[tick]=====>
< addsugar ; 0/1 ; 0/1 > < 0/1 ; 0/1 ; 0/1 >
=====[toOther]=====>
[preparingcoffee ; 0/1 ; 0/1]< 0/1 ; 0/1 ; 0/1 >
=====[tick]=====>
< preparingcoffee ; 0/1 ; 0/1 > < 0/1 ; 0/1 ; 0/1 >
=====[toOther]=====>
[cdone ; 0/1 ; 0/1]< 0/1 ; 0/1 ; 0/1 >
=====[tick]=====>
< cdone ; 10/1 ; 10/1 > < 0/1 ; 0/1 ; 0/1 >
```

Alternatively, to obtain the symbolic path, use the following command.

```maude
MaudeSE> show smt-path symbolic .

Constrained Term:
 Term: [idle ; V0:Real ; V1:Real]< V2:Real ; V3:Real ; V4:Real >
 Constraint: not (not V0:Real === X1:Real or ...)
=====[tick]=====>
Constrained Term:
 Term: < idle ; #6-V35:Real ; #7-V36:Real > < #8-V37:Real ; #9-V38:Real ; #10-V39:Real >
 Constraint: not (not V0:Real === X1:Real or ...)
=====[toAddsugar]=====>
...
```

### State Space Reduction

MaudeSE supports a well-known state space reduction technique for symbolic reachability analysis: folding. 
To illustrate this method, consider the following search command, which fails to terminate due to an infinite number of states.

```maude
MaudeSE> smt-search [1] < idle ; 0/1 ; 0/1 > < P1:Real ; P2:Real ; P3:Real > 
                    =>* < L:Location ; X':Real ; Y':Real > < P1:Real ; P2:Real ; P3:Real > 
  such that X':Real > Y':Real and P1:Real >= 0/1 and P2:Real >= 0/1 and P3:Real >= 0/1 and 
            2/1 * P1:Real > P2:Real using QF_LRA .
```

To reduce the number of symbolic states, use the `{fold}` option with the `smt-search` command. 
For example, the following command makes the state space finite, enabling an exhaustive search that correctly 
reports the absence of such a solution.

```maude
MaudeSE> {fold} smt-search [1] < idle ; 0/1 ; 0/1 > < P1:Real ; P2:Real ; P3:Real > 
             =>* < L:Location ; X':Real ; Y':Real > < P1:Real ; P2:Real ; P3:Real > 
  such that X':Real > Y':Real and P1:Real >= 0/1 and P2:Real >= 0/1 and P3:Real >= 0/1 and 
            2/1 * P1:Real > P2:Real using QF_LRA .

No more solutions.
```

---

## Satisfiability Checking

MaudeSE supports satisfiability checking under various SMT theories. 
For example, you can check whether the following formula is satisfiable under the theory of nonlinear arithmetic.

```maude
MaudeSE> check X:Real * X:Real + X:Real * Y:Real > 12/5 using QF_NRA .
result: sat
```

You can also get the model using the `show model` command.

```maude
MaudeSE> show model .

  assignment:
    X:Real |--> -1/1
    Y:Real |--> -3/1
```

---

(standalone-usage-notes)=
## Standalone Executable Usage Notes

If you are using the MaudeSE standalone executable, you need to manually load the MaudeSE meta-interpreter file [maude-se-meta.maude](https://github.com/postechsv/maude-se/blob/main/src/maude-se-meta.maude), which depends on [smt-check.maude](https://github.com/postechsv/maude-se/blob/main/src/smt-check.maude).

For example, to load the coffee machine example, load the following Maude files:

```console
$ ./maude-se-z3 coffee.maude smt-check.maude maude-se-meta.maude
```

```{warning}
The standalone version currently supports quantifier-free Boolean, Integer, and Real theories. Therefore, examples that include SMT symbols from other theories (e.g., Array), or uninterpreted symbols used in the {doc}`cmds`, {doc}`interface`, and {doc}`examples` sections, are not supported.
```

---

```{bibliography}
```