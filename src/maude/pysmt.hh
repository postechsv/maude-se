//
//      Class for generating SMT variables, version for Python support.
//
#ifndef _py_smt_hh_
#define _py_smt_hh_

class VariableGenerator : public SMT_EngineWrapper
{
public:
  VariableGenerator(const SMT_Info& smtInfo);
  ~VariableGenerator();
  //
  //	Virtual functions for SMT solving.
  //
  Result assertDag(DagNode* dag);
  Result checkDag(DagNode* dag);
  void clearAssertions();
  void push();
  void pop();

  VariableDagNode* makeFreshVariable(Term* baseVariable, const mpz_class& number);

private:
  const SMT_Info& smtInfo;
};

#endif
