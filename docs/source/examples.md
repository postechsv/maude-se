# More examples 

We provide a zip file ({download}`maude-se-examples.zip <assets/maude-se-examples.zip>`), containing example files.

| File | Description |
| ------ | ------ |
| smt-check-ex.maude | A Maude file containing examples for the `check` command |
| smt-search-ex.maude | A Maude file containing examples for the `smt-search` command |
| smt-check-meta-ex.maude | A Maude file containing examples for the `metaSmtCheck` function |
| smt-search-meta-ex.maude | A Maude file containing examples for the `metaSmtSearch` function |


## smt-check-ex.maude

This file demonstrates the usage of `check` with various theories and formulas, including both interpreted and uninterpreted symbols.
It contains four functional modules:

* `SIMPLE` 
* `EUF` 
* `EUF-ARRAY`
* `EUF-XOR` 


**1. The `SIMPLE` module**: A minimal example that imports the INTEGER module.

```maude
fmod SIMPLE is
  pr INTEGER .
  pr META-SMT-CHECK .
endfm
```

**2. The `EUF` module**: Defines a new sort `A` and declares `f` as an uninterpreted function symbol.

```maude
fmod EUF is
  pr BOOLEAN .

  sort A .

  op _===_ : A A -> Boolean .
  op f : A -> A [ctor metadata "smt euf"] .
endfm
```

**3. The `EUF-ARRAY` module**: Connects interpreted symbols from the SMT theory to Maude operators.

```maude
fmod ARRAY is
  pr REAL-INTEGER .

  sort Array{Integer,Integer} .

  op _===_ : Array{Integer,Integer} Array{Integer,Integer} -> Boolean .
  op select : Array{Integer,Integer} Integer -> Integer [metadata "smt array:select"] .
  op _[_] : Array{Integer,Integer} Integer -> Integer [metadata "smt array:select"] .
  op store : Array{Integer,Integer} Integer Integer -> Array{Integer,Integer} [metadata "smt array:store"] . 
endfm
```

**4. The `EUF-XOR` module**: Contains a single uninterpreted symbol.

```maude
fmod EUF-XOR is
  pr INTEGER .
  op _xor_ : Integer Integer -> Integer [metadata "smt euf"] .
endfm
```

As shown in the `EUF` and `EUF-XOR` modules, uninterpreted symbols can be declared by defining operators and annotating them with the `metadata` attribute. 
For example, the uninterpreted symbol `f` in `EUF` is annotated with metadata `smt euf`, indicating that the operator is both an SMT symbol (`smt`) 
and an uninterpreted function (`euf`).

Another module, `ARRAY`, shows how to connect interpreted symbols to Maude using the `metadata` attribute.
For example, the metadata `smt array:select` is used to denote the interpreted symbol `select` from the `array` theory.
Additionally, a single interpreted symbol can be connected to multiple Maude operators. 
In the ARRAY module, for instance, `select` is associated with both `select` and `_[_]`.

```{important}
To connect interpreted symbols to Maude, you need to extend or implement an SMT [converter](https://github.com/postechsv/maude-se/tree/main/src/pysmt/converter).
```

As usual, we can check the satisfiability of various formulas under the EUF and Array theories. 
For example, the following formula can be checked using the QF_UF logic. Additional example commands are available in the file.

```maude
MaudeSE> check in EUF : f(f(X:A)) === X:A and f(X:A) === Y:A 
                        and not (X:A === Y:A) using QF_UF .
result: sat
```

<!-- ```
red metaSmtCheck(upModule('INTEGER, false), upTerm(X:Integer > 4), 'QF_LRA, true) .
red metaSmtCheck(upModule('REAL, false), upTerm(X:Real > 4/3), 'QF_LRA, true) .
red metaSmtCheck(upModule('BOOLEAN, false), upTerm(B:Boolean), 'QF_LRA, true) .
red metaSmtCheck(upModule('EUF, false), upTerm(f(f(X)) === X and f(X) === Y and not (X === Y)), 'QF_UF) .
red metaSmtCheck(upModule('ARRAY, false), upTerm(select(A1, X) === X and store(A1, X, Y) === A1), 'QF_AUFLIA) .
red metaSmtCheck(upModule('ARRAY, false), upTerm(A1[X] === X and store(A1, X, Y) === A1), 'QF_AUFLIA) .
red metaSmtCheck(upModule('EUF-XOR, false), upTerm(not(N === 0) and 0 <= N and 0 <= (N xor 255) and N < 256 and not (0 <= (N xor 255))), 'QF_UFLIA) .
red metaSmtCheck(upModule('EUF-XOR, false), upTerm(0 <= (N xor 255)), 'QF_UFLIA) .
red metaSmtCheck(upModule('EUF-XOR, false), upTerm(0 <= (N xor 255)), 'QF_UFLIA, true) . --- generating a satisfying assignment
``` -->

---

## smt-check-meta-ex.maude

This file demonstrates the usage of `metaSmtCheck` with several usage examples.
Satisfiability checking can be performed as explained in {doc}`interface`.


<!-- We can run the `smt-check-ex.maude` file with CVC5 using the following command:
```
~/maude-se-linux$ maude-se smt-check-ex.maude -s cvc5
```

The results are as follows.

```
==========================================
reduce in SIMPLE : metaSmtCheck(upModule('INTEGER, false), upTerm(X:Integer > (4).Integer), 'QF_LRA, true) .
rewrites: 1
result SmtCheckResult: {'X:Integer |-> '5.Integer}
==========================================
reduce in SIMPLE : metaSmtCheck(upModule('REAL, false), upTerm(X:Real > 4/3), 'QF_LRA, true) .
rewrites: 1
result SmtCheckResult: {'X:Real |-> '7/3.Real}
==========================================
reduce in SIMPLE : metaSmtCheck(upModule('BOOLEAN, false), upTerm(B:Boolean), 'QF_LRA, true) .
rewrites: 1
result SmtCheckResult: {'B:Boolean |-> 'true.Boolean}
==========================================
reduce in EUF-CHECK : metaSmtCheck(upModule('EUF, false), upTerm(f(f(X)) === X and f(X) === Y and not X === Y), 'QF_UF) .
rewrites: 4
result Bool: (true).Bool
==========================================
reduce in ARRAY-CHECK : metaSmtCheck(upModule('ARRAY, false), upTerm(select(A1, X) === X and store(A1, X, Y) === A1), 'QF_AUFLIA) .
rewrites: 4 in 1ms cpu (1ms real) (2619 rewrites/second)
result Bool: (true).Bool
==========================================
reduce in ARRAY-CHECK : metaSmtCheck(upModule('ARRAY, false), upTerm(A1[X] === X and store(A1, X, Y) === A1), 'QF_AUFLIA) .
rewrites: 4
result Bool: (true).Bool
==========================================
reduce in EUF-XOR-CHECK : metaSmtCheck(upModule('EUF-XOR, false), upTerm(not N === (0).Integer and (0).Integer <= N and (0).Integer <= (N xor (255).Integer) and N < (256).Integer and not (0).Integer <= (N xor (255).Integer)), 'QF_UFLIA) .
rewrites: 4
result Bool: (false).Bool
==========================================
reduce in EUF-XOR-CHECK : metaSmtCheck(upModule('EUF-XOR, false), upTerm((0).Integer <= (N xor (255).Integer)), 'QF_UFLIA) .
rewrites: 4
result Bool: (true).Bool
==========================================
reduce in EUF-XOR-CHECK : metaSmtCheck(upModule('EUF-XOR, false), upTerm((0).Integer <= (N xor (255).Integer)), 'QF_UFLIA, true) .
rewrites: 1
result SmtCheckResult: {('N:Integer |-> '1.Integer),{"_xor_" |-> "(lambda ((_arg_1 Int) (_arg_2 Int)) 0)"}}
==========================================
``` -->

---

(smt-search-ex)=
## smt-search-ex.maude

This file includes examples demonstrating the use of `smt-search` with the `gcd` and `robot` modules.

```maude
mod GCD is
  pr INTEGER .

  sorts GcdResult .
  op gcd : Integer Integer -> GcdResult [ctor] .
  op return : Integer -> GcdResult [ctor] .

  vars X Y : Integer .

  crl [r1] : gcd(X, Y) => gcd(X - Y, Y) if X > Y = true [nonexec] .
  crl [r2] : gcd(X, Y) => gcd(X, Y - X) if X < Y = true [nonexec] .
   rl [r3] : gcd(X, X) => return (X) .
endm
```

The following command searches for the first solution term that matches `return(J)` and satisfies
the consition `I < 9 and I > 0`, starting from the initial term `gcd(10, I)` under the `QF_LRA` logic.

```maude
MaudeSE> smt-search [1] in GCD : gcd(10, I:Integer) =>* return(J:Integer) 
                such that I:Integer > 0 and I:Integer < 9 using QF_LRA .

Solution 1 (state 3)

Symbolic state:
 return(#5-V5:Integer)

Constraint:
 V0:Integer === 10 and V1:Integer === I:Integer and ...

Substitution:
 V2:Integer <-- #5-V5:Integer

Assignment:
 #1-V12:Integer <-- 5
 #2-V13:Integer <-- 5
 ...
 I:Integer <-- 5
 J:Integer <-- 5
 V0:Integer <-- 10
 V1:Integer <-- 5
 V2:Integer <-- 5

Concrete state:
 return(5)
````

```maude
omod ROBOT is
  pr REAL .

  class Robot | pos : Vector, vel : Vector, acc : Vector, time : Real .

  sort Vector .
  op [_,_] : Real Real -> Vector [ctor] .
endom

omod ROBOT-DYNAMICS is
  pr ROBOT .

  vars O : Oid .
  vars CONST CONST' : Boolean .
  vars PX VX AX PY VY AY PX' VX' AX' PY' VY' AY' T T' TAU : Real .

  crl [move]:
      < O : Robot | pos  : [PX,  PY], vel  : [VX,  VY],
                    acc  : [AX,  AY], time : T >
   => < O : Robot | pos  : [PX', PY'], vel  : [VX', VY'],
                    time : T + TAU >
   if TAU >= 0/1 and VX' === VX + AX * TAU and VY' === VY + AY * TAU
  and PX' === 1/2 * AX * TAU * TAU + VX * TAU + PX
  and PY' === 1/2 * AY * TAU * TAU + VY * TAU + PY = true [nonexec] .

  crl [accX]:
      < O : Robot | acc : [AX,  AY] >
   => < O : Robot | acc : [AX', AY] >
   if AY === 0/1 = true [nonexec] .

  crl [accY]:
      < O : Robot | acc : [AX, AY] >
   => < O : Robot | acc : [AX, AY'] >
   if AX === 0/1 = true [nonexec] .

  op r : -> Oid [ctor] .
endom
```

This command searches for the first solution, using the `QF_NRA` logic, that matches the goal pattern `< r : Robot | pos : [NPX, NPY], ATTRSET >` 
and satisfies the consition `NPX === 10/1 and NPY === 10/1`, starting from an initial term where all its attributes are set to zeros.

```maude
MaudeSE> smt-search [1] in ROBOT-DYNAMICS :
              < r : Robot | pos : [IPX:Real, IPY:Real], vel : [IVX:Real, IVY:Real], 
                            acc : [IAX:Real, IAY:Real], time : CLK:Real >
          =>* < r : Robot | pos : [NPX:Real, NPY:Real], ATTRSET:AttributeSet > 
    such that NPX:Real === 10/1 and NPY:Real === 10/1 
          and IPX:Real === 0/1 and IPY:Real === 0/1 and IVX:Real === 0/1 and IVY:Real === 0/1 
          and IAX:Real === 0/1 and IAY:Real === 0/1 and CLK:Real === 0/1 using QF_NRA .

Solution 1 (state 180)

Symbolic state:
 < r : Robot | pos : [#35-V24:Real, #36-V25:Real], vel : [#37-V26:Real, #38-V27:Real], 
               acc : [#39-V28:Real, #40-V29:Real], time : #41-V30:Real >

Constraint:
 V0:Real === IPX:Real and V1:Real === IPY:Real and ...

Substitution:
 ATTRSET:AttributeSet <-- vel : [#37-V26:Real, #38-V27:Real], ...
 V7:Real <-- #35-V24:Real
 V8:Real <-- #36-V25:Real

Assignment:
 #1-V15:Real <-- 0/1
 #10-V28:Real <-- 0/1
 ...

Concrete state:
 < r : Robot | pos : [10/1, 10/1], vel : [20/1, 20/3], acc : [20/1, 0/1], time : 2/1 >
````

---

## smt-search-meta-ex.maude

This file demonstrates the usage of `metaSmtSearch`, corresponding to the two commands described in the previous {ref}`section <smt-search-ex>`.

---

## Module restrictions

Currently, we impose several restrictions on rewrite rules and equations:
* The condition of a rewrite rule must be decomposed into pure SMT and non-SMT conditions.
* Equations must not contradict or conflict with SMT theories. 
* When defining and connecting SMT symbols using metadata, equational axiom attributes (e.g., assoc, comm, id) must not be used together with metadata attributes.