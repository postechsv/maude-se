#ifndef SMT_MANAGER_HH
#define SMT_MANAGER_HH
#include "smtEngineWrapperEx.hh"
#include "smtInterface.hh"

#ifdef USE_CVC4
#include "cvc4.hh"
#elif defined(USE_YICES2)
#include "yices2.hh"
#elif defined(USE_Z3)
#include "z3.hh"
#elif defined(USE_PYSMT)
#include "pysmt.hh"
#else

// Code for no SMT support case.
class VariableGenerator : public SmtEngineWrapperEx<void *, void *>
{
public:
  VariableGenerator(const SMT_Info &smtInfo);
  VariableGenerator(const SMT_Info &smtInfo, Connector *conn);
  ~VariableGenerator();

  // functions for SMT solving.
  Result assertDag(DagNode *dag);
  Result checkDag(DagNode *dag);
  // Result checkDagWithResult(DagNode* dag);
  void clearAssertions();
  void push();
  void pop();
  void reset();
  SmtModel *getModel();
  void setLogic(const char *logic);

  VariableDagNode *makeFreshVariable(Term *baseVariable, const mpz_class &number);

  inline Converter *getConverter() { return nullptr; };
  inline Connector *getConnector() { return nullptr; };
};

#endif
#endif
