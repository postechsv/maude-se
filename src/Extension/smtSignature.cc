//
//	List of all recognized constructors in metalevel.
//	Format:
//		MACRO(symbols name, symbols C++ class, required type flags, number of args)
//
//

MACRO(unknownResultSymbol, Symbol, 0, 0)
MACRO(satAssnSetSymbol, Symbol, 0, 1)
MACRO(emptySatAssnSetSymbol, Symbol, 0, 0)
MACRO(concatSatAssnSetSymbol, Symbol, 0, 2)
MACRO(intSatAssnSymbol, Symbol, 0, 2)
MACRO(boolSatAssnSymbol, Symbol, 0, 2)
MACRO(realSatAssnSymbol, Symbol, 0, 2)

MACRO(integerSymbol, SMT_NumberSymbol, 0, 0)
MACRO(realSymbol, SMT_NumberSymbol, 0, 0)

MACRO(boolVarSymbol, Symbol, 0, 1)
MACRO(intVarSymbol, Symbol, 0, 1)
MACRO(realVarSymbol, Symbol, 0, 1)

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

MACRO(autoLogicSymbol, Symbol, 0, 0)