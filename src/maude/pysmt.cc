//
//      Implementation for class Python VariableGenerator
//

//      utility stuff
#include "macros.hh"
#include "SMT.hh"
#include "variableGenerator.hh"

VariableGenerator::VariableGenerator(const SMT_Info& smtInfo)
  : smtInfo(smtInfo){
    Verbose("PySmt init");
  }

VariableGenerator::~VariableGenerator(){}

VariableGenerator::Result
VariableGenerator::assertDag(DagNode* dag)
{
  IssueWarning("PySmt currently does not implement checkSat function - giving up.");
  return SAT_UNKNOWN;
}

VariableGenerator::Result
VariableGenerator::checkDag(DagNode* dag)
{
  IssueWarning("PySmt currently does not implement checkSat function - giving up.");
  return SAT_UNKNOWN;
}

inline void
VariableGenerator::clearAssertions(){}

inline void
VariableGenerator::push(){}

inline void
VariableGenerator::pop(){}