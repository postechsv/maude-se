# SMT Interface

## Metalevel Functions

MaudeSE provides three meta-level functions defined in [smt-check.maude](https://github.com/postechsv/maude-se/blob/main/src/smt-check.maude): 

* {ref}`metaSmtCheck`
* {ref}`metaSmtSearch`
* {ref}`metaSmtSearchPath`

(metaSmtCheck)=
### metaSmtCheck

This function checks the satisfiability of SMT formulas, similar to `metaCheck`, but supports various theories 
(including uninterpreted functions) and can generate satisfying assignments.

The usage of `metaSmtCheck` is similar to that of `metaCheck`, except that it takes an SMT logic and a Boolean flag as additional arguments. 
The flag determines whether to generate a satisfying assignment when the formula is satisfiable.

```maude
op metaSmtCheck : Module Term Logic -> SmtCheckResult .
eq metaSmtCheck(M:Module, TERM:Term, LOGIC:Logic) 
 = metaSmtCheck(M:Module, TERM:Term, LOGIC:Logic, false) .
  
op metaSmtCheck : Module Term Bool -> SmtCheckResult .
eq metaSmtCheck(M:Module, TERM:Term, FLAG:Bool) 
 = metaSmtCheck(M:Module, TERM:Term, auto, FLAG:Bool) .

op metaSmtCheck : Module Term Logic Bool -> SmtCheckResult
  [special (id-hook MetaLevelSmtOpSymbol    (metaSmtCheck)
            ...)] .
```

The check result is returned as a term of sort `SmtCheckResult`, which can be one of `unknown`, `true`, `false`, or `{_}`.
The `{_}` symbol represents a set of assignments, where each assignment is expressed as a term of the form `_|->_`.

```maude
sort SmtCheckResult .
subsort Bool < SmtCheckResult .

op unknown : -> SmtCheckResult [ctor] .
op {_} : SatAssignmentSet -> SmtCheckResult [ctor] .

sort SatAssignment .
op _|->_ : Term Term -> SatAssignment [ctor] .

sort SatAssignmentSet .
subsort SatAssignment < SatAssignmentSet .
op empty : -> SatAssignmentSet [ctor] .
op _,_ : SatAssignmentSet SatAssignmentSet -> SatAssignmentSet [ctor comm assoc id: empty] .
```

(metaSmtSearch)=
### metaSmtSearch

This function performs symbolic reachability analysis using rewriting modulo SMT, similar to Maude's `metaSmtSearch`, but with additional support for folding.

The usage of `metaSmtSearch` is similar to the original, except that it takes an SMT logic, a folding flag, and a merging flag as additional arguments.

```maude
op  metaSmtSearch : Module Term Term Condition Qid Bound Nat Qid Bool Bool ~> SmtResult2? .
ceq metaSmtSearch(M, T, T', COND, STEP, B, N, LOGIC, FOLD, MERGE)
  = metaSmtSearch(transform(M, VN + N'), ...)
if {AT, CD, VN} := abst(M, T, 0) /\ {AT', CD', N'} := abst(M, T', VN)
/\ {CND, CND'} := sep(COND) /\ SMT := $getSmt(CND') .

op metaSmtSearch : Module Term Term Condition Qid Bound Nat Qid ~> SmtResult2? .
eq metaSmtSearch(M, T, T', COND, STEP, B, N, LOGIC)
 = metaSmtSearch(M, T, T', COND, STEP, B, N, LOGIC, false, false) .

op metaSmtSearch : Module Term Term Condition Qid Bound Nat Qid Bool ~> SmtResult2? .
eq metaSmtSearch(M, T, T', COND, STEP, B, N, LOGIC, FOLD)
 = metaSmtSearch(M, T, T', COND, STEP, B, N, LOGIC, FOLD, false) .

op metaSmtSearch : Module Term Term Condition Term Qid Nat Bound Nat Qid Bool Bool ~> SmtResult2?
  [special (id-hook MetaLevelSmtOpSymbol     (metaSmtSearch)
            ...)] .
```
 
(metaSmtSearchPath)=
### metaSmtSearchPath

This function takes the same argument as `metaSmtSearch` and performs the same search, but returns a path to the solution state.

---

## Core Maude Functions

MaudeSE provides two core Maude-level functions, both defined in [smt-check.maude](https://github.com/postechsv/maude-se/blob/main/src/smt-check.maude):

* {ref}`smtCheck`
* {ref}`simplifyFormula`

(smtCheck)=
### smtCheck

This function determines the satisfiability of an SMT formula. Its usage is similar to that of `metaSmtCheck`.


```maude
op smtCheck : BooleanExpr Logic -> SmtCheckResult .
eq smtCheck(B:BooleanExpr, LO:Logic) 
 = smtCheck(B:BooleanExpr, LO:Logic, false) .

op smtCheck : BooleanExpr Bool -> SmtCheckResult .
eq smtCheck(B:BooleanExpr, FLAG:Bool) 
 = smtCheck(B:BooleanExpr, auto, FLAG:Bool) .

op smtCheck : BooleanExpr -> SmtCheckResult .
eq smtCheck(B:BooleanExpr) 
 = smtCheck(B:BooleanExpr, false) .

op smtCheck : BooleanExpr Logic Bool -> SmtCheckResult 
    [special (id-hook SmtOpSymbol (smtCheck))] .
```

(simplifyFormula)=
### simplifyFormula

This function returns a simplified yet equivalent SMT formula.

```maude
op simplifyFormula : BooleanExpr -> BooleanExpr [special (id-hook SmtOpSymbol (simplify))] .
op simplifyFormula : IntegerExpr -> IntegerExpr [special (id-hook SmtOpSymbol (simplify))] .
op simplifyFormula : RealExpr -> RealExpr [special (id-hook SmtOpSymbol (simplify))] .
```

---

(generic)=
## Generic Interface

```{important}
This section will be updated shortly.
```