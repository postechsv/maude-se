#ifndef SMT_MANAGER_HH
#define SMT_MANAGER_HH
#include "smtManagerAux.hh"
#include "smtManagerFactory.hh"
#include "smtInterface.hh"
#include "variableGenerator.hh"

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
class SmtManager : public VariableGenerator
{
public:
    SmtManager(const SMT_Info& smtInfo);
    SmtManager(const SMT_Info& smtInfo, Connector* conn);
    ~SmtManager();

    // functions for SMT solving.
    Result assertDag(DagNode* dag);
    Result checkDag(DagNode* dag);
    Result checkDagWithResult(DagNode* dag);
    void clearAssertions();
    void push();
    void pop();

    VariableDagNode* makeFreshVariable(Term* baseVariable, const mpz_class& number);

public:
  inline Converter* getConverter(){ return nullptr; };
  inline Connector* getConnector(){ return nullptr; };
};

class SmtManagerFactory : public SmtManagerFactoryInterface
{
public:
    inline SmtManager* create(const SMT_Info& smtInfo){ return nullptr; };
};

#endif
#endif
