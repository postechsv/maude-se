//
//	List of all recognized constructors in metalevel.
//	Format:
//		MACRO(symbols name, symbols C++ class, required type flags, number of args)
//
//
MACRO(smtFailureSymbol, Symbol, 0, 0)
MACRO(smtResultSymbol, Symbol, 0, 5)
MACRO(assignmentSymbol, Symbol, 0, 2)
MACRO(substitutionSymbol, Symbol, 0, 2)
MACRO(emptySubstitutionSymbol, Symbol, 0, 0)