# Z3 with tactics

We extend MaudeSE-Z3 to support Z3 tactics. 
We provide a new functional module `SMT-TACTIC`, containing a `Tactic` sort.

```{note}
Unlike the other old versions of MaudeSE, this version is based on Maude 3.2.1.
```

## Download

Download tactic extended MaudeSE from [here][maude-se-z3-linux-patch].

## Tactic Functional Module

Currently, we support the following tactics. See `SMT-TACTIC` module in a `smt-check.maude` for more details.

```maude
op simplify : -> Tactic [ctor] .
op qe : -> Tactic [ctor] .
op qe2 : -> Tactic [ctor] .
op ctxSolverSimplify : -> Tactic [ctor] .
op propagateInEqs : -> Tactic [ctor] .
op purifyArith : -> Tactic [ctor] .
op then : Tactic NeTacticList -> Tactic [ctor] .
```

## Usage

We can use Z3 tactic by an operator `apply`, defined in the `SMT-CHECK` module.

```maude
op apply : BooleanExpr Tactic -> BooleanExpr [special ...] .
```

### Simplify

For example, you can apply `simplify` tactic as follows:

```maude
red apply(i("a") + 1 < 1, simplify) .
red apply(b("a") === true, simplify) .
red apply(forall(i("a"), i("a") > 1), simplify) .
```

The results are as follows:

```console
==========================================
reduce in TEST : apply(i("a") + (1).Integer < (1).Integer, simplify) .
rewrites: 1
result BooleanExpr: not (0).Integer <= i("a")
==========================================
reduce in TEST : apply(b("a") === (true).Boolean, simplify) .
rewrites: 1
result BooleanVar: b("a")
==========================================
reduce in TEST : apply(forall(i("a"), i("a") > (1).Integer), simplify) .
rewrites: 1
result BooleanExpr: forall(freshIntVar(0), not freshIntVar(0) <= (1).Integer)
```

```{note}
Z3 uses de-Bruijn indexed variables for bound variables.
MaudeSE generates these indexed variables using three operators (i.e., `freshIntVar`, `freshBoolVar`, or `freshRealVar`). 
```

### Quantifier Elimination

You can apply `qe` (or similarly `qe2`) tactic as follows:

```maude
red apply(forall(i("a"), i("a") > 1), qe) .
red apply(exists(i("a"), i("a") > 1), qe) .
red apply(forall(i("a"), i("a2") > 1), qe) .
red apply(exists(i("a"), i("a2") > 1), qe) .
red apply(exists(i("a"), exists(i("a1"), i("a1") > 1)), qe) .
```

The results are as follows:

```console
==========================================
reduce in TEST : apply(forall(i("a"), i("a") > (1).Integer), qe) .
rewrites: 1
result Boolean: (false).Boolean
==========================================
reduce in TEST : apply(exists(i("a"), i("a") > (1).Integer), qe) .
rewrites: 1
result Boolean: (true).Boolean
==========================================
reduce in TEST : apply(forall(i("a"), i("a2") > (1).Integer), qe) .
rewrites: 1
result BooleanExpr: not i("a2") <= (1).Integer
==========================================
reduce in TEST : apply(exists(i("a"), i("a2") > (1).Integer), qe) .
rewrites: 1
result BooleanExpr: not i("a2") <= (1).Integer
==========================================
reduce in TEST : apply(exists(i("a"), exists(i("a1"), i("a1") > (1).Integer)), qe) .
rewrites: 1
result Boolean: (true).Boolean
```

### And

You can combine multiple tactics using `&` as follow: 

```maude
red apply(forall(i("a"), forall(i("a2"), i("a") + i("a2") + i("a3") > 1)),
    then(qe, ctxSolverSimplify qe qe2 ctxSolverSimplify propagateInEqs purifyArith)) .
```

```console
reduce in TEST : apply(forall(i("a"), forall(i("a2"), i("a") + i("a2") + i("a3") > (1).Integer)), 
    then(qe, ctxSolverSimplify qe qe2 ctxSolverSimplify propagateInEqs purifyArith)) .
rewrites: 1
result Boolean: (false).Boolean
```

## Bug Report

To report bugs (or provide any suggestions), please contact [rgyen@postech.ac.kr](mailto:rgyen@postech.ac.kr).

[maude-se-z3-linux-patch]: https://tinyurl.com/2p9jdc5n