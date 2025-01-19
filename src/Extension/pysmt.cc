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
#include "extGlobal.hh"

VariableGenerator::VariableGenerator(const SMT_Info& smtInfo)
  : SmtEngineWrapperEx(smtInfo), conn(nullptr){
    Verbose("PySmt init");

    vg = smtManagerFactory->create(smtInfo);
    conn = vg->getConnector();
    conv = vg->getConverter();
  }

VariableGenerator::VariableGenerator(const SMT_Info& smtInfo, Connector* conn)
  : SmtEngineWrapperEx(smtInfo), conn(conn), conv(conn->get_converter()), vg(nullptr) {
    Verbose("PySmt init");
  }

VariableGenerator::~VariableGenerator(){
  if (vg) delete vg;
  else {
    delete conn;
    delete conv;
  }
}

VariableGenerator::Result
VariableGenerator::assertDag(DagNode* dag)
{
  SmtTerm* o = conv->dag2term(dag);
  std::vector<SmtTerm*> formulas;
  formulas.push_back(o);

  if(conn->check_sat(formulas)){
    return SAT;
  }
  return UNSAT;
}

VariableGenerator::Result
VariableGenerator::checkDag(DagNode* dag)
{
  push();
  Result r = assertDag(dag);
  pop();

  return r;
}

inline void
VariableGenerator::clearAssertions(){ conn->reset(); }

inline void
VariableGenerator::push(){ conn->push(); }

inline void
VariableGenerator::pop(){ conn->pop(); }

SmtModel* VariableGenerator::getModel(){
  return conn->get_model();
}

inline void 
VariableGenerator::setLogic(const char* logic){
  conn->set_logic(logic);
}

// SmtManagerFactory::SmtManagerFactory(){}