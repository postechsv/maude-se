#ifndef ABSTRACT_SMT_MANAGER_HH
#define ABSTRACT_SMT_MANAGER_HH

// utility stuff
#include "macros.hh"
#include "vector.hh"

// forward declarations
#include "interface.hh"
#include "core.hh"
#include "freeTheory.hh"
#include "variable.hh"
#include "builtIn.hh"
#include "mixfix.hh"

// interface class definitions
#include "symbol.hh"
#include "term.hh"

// core class definitions
#include "rewritingContext.hh"
#include "symbolMap.hh"

// variable class definitions
#include "variableSymbol.hh"
#include "variableDagNode.hh"

// free theory class definitions
#include "freeDagNode.hh"


// builtin class definition
#include "bindingMacros.hh"

#include "SMT_Info.hh"
#include "SMT_Symbol.hh"
#include "SMT_NumberSymbol.hh"
#include "SMT_NumberDagNode.hh"
#include "SMT_EngineWrapper.hh"


#include "extensionSymbol.hh"
#include "smtCheckSymbol.hh"
#include "tacticApplySymbol.hh"
#include "smtInterface.hh"
#include <map>

/*
 * Extension of SMT_EngineWrapperInterface
 */
class SmtEngineWrapperEx : virtual public SMT_EngineWrapper
{
public:

    /*
     * Dag2Term should throw ExtensionException when any error occurs.
     *
     * makeExtensionVariable : User should check smtCheckerSymbol is null or not.
     * variableGenerator should know its dag parameter type before calling.
     * checkDagExtension's result is different type compare to SMT_EngineWrapper result type.
     */
    virtual Connector* getConnector() = 0;
    virtual Converter* getConverter() = 0;
};

#endif
