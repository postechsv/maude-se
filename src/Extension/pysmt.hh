//
//      Class for generating SMT variables, version for Python support.
//
#ifndef _py_smt_hh_
#define _py_smt_hh_

#include "smtManagerAux.hh"
#include "smtManagerFactory.hh"
#include "smtInterface.hh"
#include "variableGenerator.hh"

// #include "SMT_EngineWrapper.hh"

class SmtManager :
        public SmtManagerAux<void*,void*>,
        public VariableGenerator {
    NO_COPYING(SmtManager);
public:
  SmtManager(const SMT_Info& smtInfo);
  SmtManager(const SMT_Info& smtInfo, Connector* conn);
  ~SmtManager();
  //
  //	Virtual functions for SMT solving.
  //
  Result assertDag(DagNode* dag);
  Result checkDag(DagNode* dag);
  void clearAssertions();
  void push();
  void pop();

  // VariableDagNode* makeFreshVariable(Term* baseVariable, const mpz_class& number);
public:

  // legacy
  DagNode* Term2Dag(void* exp, ExtensionSymbol* extensionSymbol, ReverseSmtManagerVariableMap* rsv) noexcept(false){ return nullptr; };
  void* Dag2Term(DagNode* dag, ExtensionSymbol* extensionSymbol) noexcept(false){ return nullptr; };
  DagNode* generateAssignment(DagNode* dagNode, SmtCheckerSymbol* smtCheckerSymbol){ return nullptr; };
  DagNode* simplifyDag(DagNode* dagNode, ExtensionSymbol* extensionSymbol){ return nullptr; };
  DagNode* applyTactic(DagNode* dagNode, DagNode* tacticTypeDagNode, ExtensionSymbol* extensionSymbol){ return nullptr; };
  void* variableGenerator(DagNode* dag, ExprType exprType){ return nullptr; };
  SmtResult checkDagContextFree(DagNode* dag, ExtensionSymbol* extensionSymbol){ return SMT_SAT_UNKNOWN; };

public:
  inline Converter* getConverter(){ return conn->get_converter(); };
  inline Connector* getConnector(){ return conn; };

private:
  Connector* conn;
};

class SmtManagerFactory : public SmtManagerFactoryInterface
{
public:
    SmtManagerFactory();
};

#endif
