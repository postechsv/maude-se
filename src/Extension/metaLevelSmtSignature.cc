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