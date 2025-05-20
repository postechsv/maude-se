noinst_LIBRARIES = libExtension.a

libExtension_a_CPPFLAGS = \
	-I$(top_srcdir)/src/Utility \
	-I$(top_srcdir)/src/Temporal \
	-I$(top_srcdir)/src/Interface \
	-I$(top_srcdir)/src/Core \
	-I$(top_srcdir)/src/Variable \
	-I$(top_srcdir)/src/FullCompiler \
	-I$(top_srcdir)/src/Higher \
	-I$(top_srcdir)/src/CUI_Theory \
	-I$(top_srcdir)/src/S_Theory \
	-I$(top_srcdir)/src/NA_Theory \
	-I$(top_srcdir)/src/FreeTheory \
	-I$(top_srcdir)/src/ObjectSystem \
	-I$(top_srcdir)/src/Mixfix \
	-I$(top_srcdir)/src/BuiltIn \
	-I$(top_srcdir)/src/Parser \
	-I$(top_srcdir)/src/IO_Stuff \
	-I$(top_srcdir)/src/ACU_Persistent \
	-I$(top_srcdir)/src/ACU_Theory \
	-I$(top_srcdir)/src/AU_Persistent \
	-I$(top_srcdir)/src/AU_Theory \
	-I$(top_srcdir)/src/Meta \
	-I$(top_srcdir)/src/3rdParty \
	-I$(top_srcdir)/src/FullCompiler \
	-I$(top_srcdir)/src/StrategyLanguage \
	-I$(top_srcdir)/src/SMT

libExtension_a_SOURCES = \
	metaLevelSmtOpSymbol.cc \
	folder.cc \
	smtStateTransitionGraph.cc \
	rewriteSmtSearchState.cc \
	rewriteSmtSequenceSearch.cc \
	smtManager.cc \
	smtOpSymbol.cc \
	simplifyFormulaSymbol.cc \
	extensionSymbol.cc \
	tacticApplySymbol.cc \
	extGlobal.cc


EXTRA_DIST = \
    z3.cc \
    yices2.cc \
    cvc4.cc

noinst_HEADERS = \
    z3.hh \
    yices2.hh \
    cvc4.hh \
    nativeSmt.hh \
	smtManager.hh \
    smtOpSymbol.hh \
    simplifyFormulaSymbol.hh \
    extensionSymbol.hh \
    tacticApplySymbol.hh
