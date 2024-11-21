// utility stuff
#include "macros.hh"
#include "vector.hh"

// forward declarations
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "mixfix.hh"
#include "SMT.hh"

// interface class definitions
#include "symbol.hh"
#include "term.hh"

// variable class definitions
#include "variableDagNode.hh"
#include "variableTerm.hh"

#include "freeDagNode.hh"

// SMT class definitions
#include "SMT_Symbol.hh"
#include "SMT_NumberSymbol.hh"
#include "SMT_NumberDagNode.hh"
#include "smtManager.hh"

#ifdef USE_CVC4
#include "cvc4.cc"
#elif defined(USE_YICES2)
#include "yices2.cc"
#elif defined(USE_Z3)
#include "z3.cc"
#elif defined(USE_PYSMT)
#include "pysmt.cc"
#else

// no SMT support case.
SmtManager::SmtManager(const SMT_Info& smtInfo) : VariableGenerator(smtInfo){
    IssueWarning("No SMT solver linked at compile time.");
}

SmtManager::SmtManager(const SMT_Info& smtInfo, Connector* conn) 
    : VariableGenerator(smtInfo) {
    IssueWarning("No SMT solver linked at compile time.");
}

SmtManager::~SmtManager(){}

SmtManager::Result SmtManager::assertDag(DagNode* dag){
    return SAT_UNKNOWN;
}

SmtManager::Result SmtManager::checkDag(DagNode* dag){
    return SAT_UNKNOWN;
}

SmtManager::Result SmtManager::checkDagWithResult(DagNode *dag){
    return SAT_UNKNOWN;
}

void SmtManager::push(){}

void SmtManager::pop(){}

void SmtManager::clearAssertions(){}

#endif