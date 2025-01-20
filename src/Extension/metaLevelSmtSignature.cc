//
//	List of all recognized constructors in metalevel.
//	Format:
//		MACRO(symbols name, symbols C++ class, required type flags, number of args)
//
//

// check
MACRO(unknownResultSymbol, Symbol, 0, 0)
MACRO(satAssnSetSymbol, Symbol, 0, 1)
MACRO(emptySatAssnSetSymbol, Symbol, 0, 0)
MACRO(concatSatAssnSetSymbol, Symbol, 0, 2)
MACRO(satAssnSymbol, Symbol, 0, 2)
MACRO(satAssnAnySymbol, Symbol, 0, 2)

// search
MACRO(smtFailureSymbol, Symbol, 0, 0)
MACRO(smtResultSymbol, Symbol, 0, 5)
MACRO(assignmentSymbol, Symbol, 0, 2)
MACRO(substitutionSymbol, Symbol, 0, 2)
MACRO(emptySubstitutionSymbol, Symbol, 0, 0)


// Dag
MACRO(integerSymbol, SMT_NumberSymbol, 0, 0)
MACRO(realSymbol, SMT_NumberSymbol, 0, 0)
MACRO(notBoolSymbol, Symbol, 0, 1)
MACRO(andBoolSymbol, Symbol, 0, 2)
MACRO(xorBoolSymbol, Symbol, 0, 2)
MACRO(orBoolSymbol, Symbol, 0, 2)
MACRO(impliesBoolSymbol, Symbol, 0, 2)
MACRO(eqBoolSymbol, Symbol, 0, 2)
MACRO(neqBoolSymbol, Symbol, 0, 2)
MACRO(iteBoolSymbol, Symbol, 0, 3)

MACRO(unaryMinusIntSymbol, Symbol, 0, 1)
MACRO(plusIntSymbol, Symbol, 0, 2)
MACRO(minusIntSymbol, Symbol, 0, 2)
MACRO(divIntSymbol, Symbol, 0, 2)
MACRO(mulIntSymbol, Symbol, 0, 2)
MACRO(modIntSymbol, Symbol, 0, 2)
MACRO(ltIntSymbol, Symbol, 0, 2)
MACRO(leqIntSymbol, Symbol, 0, 2)
MACRO(gtIntSymbol, Symbol, 0, 2)
MACRO(geqIntSymbol, Symbol, 0, 2)
MACRO(eqIntSymbol, Symbol, 0, 2)
MACRO(neqIntSymbol, Symbol, 0, 2)
MACRO(iteIntSymbol, Symbol, 0, 3)
MACRO(divisibleIntSymbol, Symbol, 0, 2)

MACRO(unaryMinusRealSymbol, Symbol, 0, 1)
MACRO(plusRealSymbol, Symbol, 0, 2)
MACRO(minusRealSymbol, Symbol, 0, 2)
MACRO(divRealSymbol, Symbol, 0, 2)
MACRO(mulRealSymbol, Symbol, 0, 2)
MACRO(ltRealSymbol, Symbol, 0, 2)
MACRO(leqRealSymbol, Symbol, 0, 2)
MACRO(gtRealSymbol, Symbol, 0, 2)
MACRO(geqRealSymbol, Symbol, 0, 2)
MACRO(eqRealSymbol, Symbol, 0, 2)
MACRO(neqRealSymbol, Symbol, 0, 2)
MACRO(iteRealSymbol, Symbol, 0, 3)
MACRO(toRealSymbol, Symbol, 0, 1)
MACRO(toIntegerSymbol, Symbol, 0, 1)
MACRO(isIntegerSymbol, Symbol, 0, 1)
