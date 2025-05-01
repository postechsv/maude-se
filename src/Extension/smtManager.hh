#ifndef SMT_MANAGER_HH
#define SMT_MANAGER_HH
#include "smtEngineWrapperEx.hh"
#include "smtInterface.hh"
#include "token.hh"

class MetaLevelSmtOpSymbol;

class VariableGenerator : public SmtEngineWrapperEx
{
  NO_COPYING(VariableGenerator);

public:
  VariableGenerator(const SMT_Info &smtInfo);
  // VariableGenerator(const SMT_Info &smtInfo, Connector *conn);
  VariableGenerator(const SMT_Info &smtInfo, MetaLevelSmtOpSymbol *extensionSymbol);
  ~VariableGenerator();
  //
  //	Virtual functions for SMT solving.
  //
  Result assertDag(DagNode *dag);
  Result checkDag(DagNode *dag);
  void clearAssertions();
  void push();
  void pop();
  SmtModel *getModel();
  void setLogic(const char *logic);

  VariableDagNode *makeFreshVariable(Term *baseVariable, const mpz_class &number);

public:
  // legacy
  // DagNode *Term2Dag(SmtTerm *exp, ExtensionSymbol *extensionSymbol, ReverseSmtManagerVariableMap *rsv) noexcept(false) { return nullptr; };
  // SmtTerm *Dag2Term(DagNode *dag, ExtensionSymbol *extensionSymbol) noexcept(false) { return nullptr; };
  // DagNode *generateAssignment(DagNode *dagNode, ExtensionSymbol *extensionSymbol) { return nullptr; };
  // DagNode *simplifyDag(DagNode *dagNode, ExtensionSymbol *extensionSymbol) { return nullptr; };
  // DagNode *applyTactic(DagNode *dagNode, DagNode *tacticTypeDagNode, ExtensionSymbol *extensionSymbol) { return nullptr; };
  // SmtTerm *variableGenerator(DagNode *dag, ExprType exprType) { return nullptr; };
  // Result checkDagContextFree(DagNode *dag, ExtensionSymbol *extensionSymbol) { return SAT_UNKNOWN; };

public:
  inline Converter *getConverter() { return conv; };
  inline Connector *getConnector() { return conn; };

private:
  Connector *conn;
  Converter *conv;
};

// Common auxiliary functions for dag generation.
// Taken from Maude as a library.

bool containsSpecialChars(const char *str);
string escapeWithBackquotes(const char *str);
int encodeEscapedToken(const char *str);

#ifdef USE_CVC4
#include "cvc4.hh"
#elif defined(USE_YICES2)
#include "yices2.hh"
#elif defined(USE_Z3)
#include "z3.hh"
#elif defined(USE_PYSMT)
#include "pysmt.hh"
#else

// dummy
class DummyConverter : public Converter
{
public:
  ~DummyConverter() {};
  void prepareFor(VisibleModule *module) {};
  SmtTerm *dag2term(DagNode *dag);
  DagNode *term2dag(SmtTerm *term);
};

class DummyConnector : public Connector
{
public:
  DummyConnector(DummyConverter *conv) { conv = conv; };
  ~DummyConnector() {};
  bool check_sat(std::vector<SmtTerm *> consts);
  bool subsume(TermSubst *subst, SmtTerm *prev, SmtTerm *acc, SmtTerm *cur);
  TermSubst *mk_subst(std::map<DagNode *, DagNode *> &subst_dict);
  SmtTerm *add_const(SmtTerm *acc, SmtTerm *cur);
  SmtModel *get_model();
  void push() {};
  void pop() {};

  void print_model() {};
  void set_logic(const char *logic);
  void reset() {};

  Converter *get_converter() { return conv; };

private:
  DummyConverter *conv;
};

class DummySmtManagerFactory : public SmtManagerFactory
{
public:
  VariableGenerator *create(const SMT_Info &smtInfo);
};

#endif
#endif
