//
//      Class for generating SMT variables, version for Python support.
//
#ifndef _py_smt_hh_
#define _py_smt_hh_

#include "smtEngineWrapperEx.hh"
#include "smtManagerFactory.hh"
#include "smtInterface.hh"

// #include "SMT_EngineWrapper.hh"

class VariableGenerator :
        public SmtEngineWrapperEx<void*,void*> {
    NO_COPYING(VariableGenerator);
public:
  VariableGenerator(const SMT_Info& smtInfo);
  VariableGenerator(const SMT_Info& smtInfo, Connector* conn);
  ~VariableGenerator();
  //
  //	Virtual functions for SMT solving.
  //
  Result assertDag(DagNode* dag);
  Result checkDag(DagNode* dag);
  void clearAssertions();
  void push();
  void pop();
  SmtModel* getModel();
  void setLogic(const char* logic);

  VariableDagNode* makeFreshVariable(Term* baseVariable, const mpz_class& number);
public:

  // legacy
  DagNode* Term2Dag(void* exp, ExtensionSymbol* extensionSymbol, ReverseSmtManagerVariableMap* rsv) noexcept(false){ return nullptr; };
  void* Dag2Term(DagNode* dag, ExtensionSymbol* extensionSymbol) noexcept(false){ return nullptr; };
  DagNode* generateAssignment(DagNode* dagNode, ExtensionSymbol* extensionSymbol){ return nullptr; };
  DagNode* simplifyDag(DagNode* dagNode, ExtensionSymbol* extensionSymbol){ return nullptr; };
  DagNode* applyTactic(DagNode* dagNode, DagNode* tacticTypeDagNode, ExtensionSymbol* extensionSymbol){ return nullptr; };
  void* variableGenerator(DagNode* dag, ExprType exprType){ return nullptr; };
  Result checkDagContextFree(DagNode* dag, ExtensionSymbol* extensionSymbol){ return SAT_UNKNOWN; };

public:
  inline Converter* getConverter(){ return conv; };
  inline Connector* getConnector(){ return conn; };

private:
  Connector* conn;
  Converter* conv;
  VariableGenerator* vg;
};

// class SmtManagerFactory : public SmtManagerFactoryInterface
// {
// public:
//     SmtManagerFactory();
// };

#endif
