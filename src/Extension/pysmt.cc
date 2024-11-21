//
//      Implementation for class Python VariableGenerator
//

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

#include "pysmt.hh"

SmtManager::SmtManager(const SMT_Info& smtInfo)
  : SmtManagerAux(smtInfo), VariableGenerator(smtInfo), conn(nullptr){
    Verbose("PySmt init");
  }

SmtManager::SmtManager(const SMT_Info& smtInfo, Connector* conn)
  : SmtManagerAux(smtInfo), VariableGenerator(smtInfo), conn(conn) {
    Verbose("PySmt init");
  }

SmtManager::~SmtManager(){}

SmtManager::Result
SmtManager::assertDag(DagNode* dag)
{
  IssueWarning("PySmt currently does not implement checkSat function - giving up.");
  return SAT_UNKNOWN;
}

SmtManager::Result
SmtManager::checkDag(DagNode* dag)
{
  IssueWarning("PySmt currently does not implement checkSat function - giving up.");
  return SAT_UNKNOWN;
}

inline void
SmtManager::clearAssertions(){}

inline void
SmtManager::push(){}

inline void
SmtManager::pop(){}

SmtManagerFactory::SmtManagerFactory(){}