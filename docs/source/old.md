# Old version

MaudeSE is available in three versions: Z3, Yices2, and CVC4. To access a variant with extended Z3 tactics, visit {doc}`here <z3patch>`.

```{warning}
This version is deprecated and no longer maintained.
```

## Getting started

To use MaudeSE, please follow the instructions below:

1. Download one of the zip files from [here](#download) (e.g., maude-se-z3.tar.gz).
2. Extract the zip file to somewhere you want.
```console
$ tar -xvzf maude-se-z3.tar.gz && cd maude-se-z3
```
3. Load or include `smt-check.maude` in your own module.

(download)=
## Download

MaudeSE supports three SMT solvers: Z3, Yices2, and CVC4. 
For Yices2, two versions are available: one with MCSAT support and one without.

```{note}
For Yices to handle nonlinear theories, mcsat is required
```

| #        | Name          | Files |
|:-------------|:------------------|:------|
| 1 | maude-se-z3 | \[macOS\]\[[Linux][maude-se-z3-linux]\] |
| 2 | maude-se-yices2 | \[macOS\]\[[Linux][maude-se-yices-linux]\] |
| 3 | maude-se-yices2-mcsat | \[macOS\]\[[Linux][maude-se-yices-mcsat-linux]\] |
| 4 | maude-se-cvc4 | \[macOS\]\[[Linux][maude-se-cvc4-linux]\] |


## SMT Interface

### Full Maude Interface

The Full Maude interface of Maude SE provides `smt-search2` command for symbolic reachability analysis.

#### Search Command

Given an initial state `t`, a goal pattern `u`, and a goal condition `cond`, the following command searches for `n` states that are reachable within `m` rewrite steps from the initial state `t`, match the goal pattern `u`, and satisfy the goal condition `cond`.

```maude
(smt-search2 [n, m] in M : t =>* u such that cond.)
```

#### Usage

To see the usage of this command, refer to the old MaudeSE [paper](https://wrla2020.webs.upv.es/pre-proceedings.pdf#page=227) for more details. 
For the simple usage of the command, see the [full/robot.maude](#full-maude-robot) example. 


### Core Maude Interface

The main functionality of the Maude SE is implemented in the predefined functional module `SMT-CHECK` which also contains signatures for both SMT formulas and their solutions.

The interface contains two main functions in the module `SMT-CHECK`:
1. `smtCheck` for checking the satisfiability of an SMT formula, and 
1. `simplifyFormula` for simplifying the formula.

#### Core Maude Functions

```maude
op smtCheck : BooleanExpr -> SmtCheckResult .
eq smtCheck(BE:BooleanExpr) = smtCheck(BE:BooleanExpr, false) .
op smtCheck : BooleanExpr Bool -> SmtCheckResult [special (...)] .
op simplifyFormula : BooleanExpr -> BooleanExpr [special (...)] .
op simplifyFormula : IntegerExpr -> IntegerExpr [special (...)] .
op simplifyFormula : RealExpr -> RealExpr [special (...)] .
```

#### Usage

To see the usages of these functions, see [gcd.maude](#gcd) and [robot.maude](#robot) examples. 


## Examples

We provide three examples: robot-full.maude, gcd.maude, and robot.maude.

(full-maude-robot)=
### Full Maude Robot

```maude
(omod ROBOT-DYNAMICS is
  pr REAL .
  inc CONFIGURATION .

  sort Vector .
  op `[_`,_`] : Real Real -> Vector [ctor] .
  sorts RobotTraceItem RobotTrace .
  subsort Vector < RobotTraceItem < RobotTrace .
  op ==`(_:_`)==> : Vector Real -> RobotTraceItem [ctor] .
  op __ : RobotTrace RobotTrace -> RobotTrace [ctor assoc] .
  class Robot | pos : Vector, vel : Vector, acc : Vector, clock : Real, trace : RobotTrace .
  sort State .
  op r : -> Oid [ctor] .
  subsort Configuration < State .

  vars O : Oid .
  vars PX VX AX PY VY AY PX' VX' AX' PY' VY' AY' T T' TAU : Real .
  var ATTS : AttributeSet .
  var TRACE : RobotTrace .

  crl [move]:
      < O : Robot | pos  : [PX,  PY], 
                       vel  : [VX,  VY],
			acc : [AX, AY],
                       clock : T,
                       trace : TRACE >
   => < O : Robot | pos  : [PX', PY'], 
                       vel  : [VX', VY'],
                       clock : T + TAU, 
                       trace : TRACE ==([VX', VY'] : T + TAU)==> [PX', PY'] >
   if VX' = VX + AX * TAU
   /\ VY' = VY + AY * TAU
   /\ PX' = 1/2 * AX * TAU * TAU + VX * TAU + PX
   /\ PY' = 1/2 * AY * TAU * TAU + VY * TAU + PY 
   /\ TAU >= 0/1 = true [nonexec] . 

  crl [accX]:
      < O : Robot | acc : [AX,  AY] >
   => < O : Robot | acc : [AX', AY] >
   if AY = 0/1 [nonexec] .

  crl [accY]:
      < O : Robot | acc : [AX, AY] >
   => < O : Robot | acc : [AX, AY'] >
   if AX = 0/1 [nonexec] . 
endom)

(smt-search2 [1] 
    < r : Robot | pos  : [0/1, 0/1],
                   vel  : [0/1, 0/1],
                   acc  : [0/1, 0/1],
                   clock : 0/1,
		   trace : [0/1, 0/1] >
  =>*
    < r : Robot | pos  : [10/1, 10/1] > .)
```


To run the full-maude robot example using Z3, go to the top directory (where the exectuable is placed) and type the following command:

```console
$ ./maude-se-z3 examples/full/robot.maude
```

(gcd)=
### GCD

```maude
mod GCD is
  protecting INTEGER-EXPR .
  pr SMT-CHECK .
  pr META-LEVEL .
    
  ops a b : -> SMTVarId [ctor] .

  sort GcdResult .
  op gcd : IntegerExpr IntegerExpr -> GcdResult [ctor] .
  op ret : IntegerExpr -> GcdResult [ctor] .


  sort State .
  op {_,_} : BooleanExpr GcdResult -> State [ctor] .

  vars CONST CONST' : BooleanExpr .
  vars X Y : IntegerExpr .
  var SS : SatAssignmentSet .

  crl {CONST, gcd(X, Y)} => {simplifyFormula(CONST'), gcd(X - Y, Y)}
  if CONST' := CONST and (X > Y) === (true).Boolean / smtCheck(CONST', false) .

  crl {CONST, gcd(X, Y)} => {simplifyFormula(CONST'), gcd(X, Y - X)}
  if CONST' := CONST and (X < Y) === (true).Boolean / smtCheck(CONST', false) .

  crl {CONST, gcd(X, Y)} => {simplifyFormula(CONST'), ret(X)}
  if CONST' := CONST and (X === Y) === (true).Boolean / smtCheck(CONST', false) .
endm

search [1] {true, gcd(i(a), i(b))} =>* {CONST, ret(NN:IntegerExpr)} 
    such that CONST' := CONST and i(a) + i(b) === 27 and NN:IntegerExpr === 3 
           /\ smtCheck(CONST') = (true).Bool .
```

To run the GCD example using Z3, go to top directory (whre the executable is placed) and type the following command:

```console
$ ./maude-se-z3 examples/gcd.maude
```

(robot)=
### Robot

```maude
mod ROBOT is
  pr REAL-EXPR .
  inc CONFIGURATION .
    
  sort Robot .
  subsort Robot < Cid .
  op Robot : -> Robot [ctor format(! o)] .
    
  op pos`:_ : Vector -> Attribute [ctor gather(&)] .
  op vel`:_ : Vector -> Attribute [ctor gather(&)] .
  op acc`:_ : Vector -> Attribute [ctor gather(&)] .
  op time`:_ : RealExpr -> Attribute [ctor gather(&)] .
    
  sort Vector .
  op [_,_] : RealExpr RealExpr -> Vector [ctor] .
endm
    
mod ROBOT-DYNAMICS is
  pr ROBOT .
  pr SMT-CHECK .
  inc NAT .
  pr META-LEVEL .
    
  sort State .
  subsort Nat < SMTVarId .
  op {_,_,_} : BooleanExpr Nat Configuration -> State [ctor] .
    
  var N : Nat .
  vars O : Oid .
  vars CONST CONST' : BooleanExpr .
  vars PX VX AX PY VY AY PX' VX' AX' PY' VY' AY' T T' TAU : RealExpr .
  var ATTS : AttributeSet .
    
  crl [move]:
  {CONST, N, 
    < O : C:Robot | pos  : [PX,  PY], 
                    vel  : [VX,  VY],
                    acc  : [AX,  AY],  
                    time : T, ATTS >}
    => {CONST', N + 5, 
    < O : C:Robot | pos  : [PX', PY'], 
                    vel  : [VX', VY'],
                    acc  : [AX,  AY], 
                    time : T + TAU, ATTS >}
    if TAU := r(N)
    /\ [PX', PY'] := [r(N + 1), r(N + 2)]
    /\ [VX', VY'] := [r(N + 3), r(N + 4)]
    /\ CONST' := simplifyFormula(CONST and TAU >= toReal(0)
            and VX' === VX + AX * TAU
            and VY' === VY + AY * TAU
            and PX' === 1/2 * AX * TAU * TAU + VX * TAU + PX
            and PY' === 1/2 * AY * TAU * TAU + VY * TAU + PY) 
    /\ smtCheck(CONST') .
    
  crl [accX]:
    {CONST,  N,   < O : C:Robot | acc : [AX,  AY], ATTS >}
    => 
    {CONST', s N, < O : C:Robot | acc : [AX', AY], ATTS >}
    if AX' := r(N)
    /\ CONST' := simplifyFormula(CONST and AY === toReal(0))
    /\ smtCheck(CONST') .
    
  crl [accY]:
    {CONST,  N,   < O : C:Robot | acc : [AX, AY],  ATTS >}
    => 
    {CONST', s N, < O : C:Robot | acc : [AX, AY'], ATTS >}
    if AY' := r(N)
    /\ CONST' := simplifyFormula(CONST and AX === toReal(0)) 
    /\ smtCheck(CONST') .
endm
    
mod ROBOT-ANALYSIS is
  inc ROBOT-DYNAMICS .
    
  var N : Nat .
  vars CONST : BooleanExpr .
  vars PX VX AX PY VY AY T : RealExpr .
  var SS : SatAssignmentSet .
  var OBJ : Object .
  var ATTS : AttributeSet .
    
  op r : -> Oid [ctor] .
endm
      
search [1] 
{true,  0, 
 < r : Robot | pos  : [toReal(0), toReal(0)],
               vel  : [toReal(0), toReal(0)],
               acc  : [toReal(0), toReal(0)],
               time : toReal(0) >}
=>*
{CONST,  N, 
 < r : Robot | pos  : [PX, PY], ATTS >} such that
{SS} := smtCheck(CONST and PX === 10/1 and PY === 10/1, true) .
```

To run the robot example using Z3, go to top directory (whre the executable is placed) 
and type the following command:

```console
$ ./maude-se-z3 examples/robot.maude
```

[maude-se-z3-mac-intel]: https://tinyurl.com/4x24c4fk
[maude-se-z3-linux]: https://tinyurl.com/rvkfbume

[maude-se-yices-mac-intel]: https://tinyurl.com/2vbsbcfm
[maude-se-yices-linux]: https://tinyurl.com/59md34jd

[maude-se-yices-mcsat-mac-intel]: https://tinyurl.com/2vbsbcfm
[maude-se-yices-mcsat-linux]: https://tinyurl.com/2rvybjdx

[maude-se-cvc4-mac-intel]: https://tinyurl.com/3kthjb7s
[maude-se-cvc4-linux]: https://tinyurl.com/2b6escjh