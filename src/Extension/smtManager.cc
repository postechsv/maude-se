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

//	front end class definitions
#include "token.hh"

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
VariableGenerator::VariableGenerator(const SMT_Info &smtInfo)
{
    IssueWarning("No SMT solver linked at compile time.");
}

VariableGenerator::VariableGenerator(const SMT_Info &smtInfo, Connector *conn)
{
    IssueWarning("No SMT solver linked at compile time.");
}

VariableGenerator::~VariableGenerator() {}

VariableGenerator::Result VariableGenerator::assertDag(DagNode *dag)
{
    return SAT_UNKNOWN;
}

VariableGenerator::Result VariableGenerator::checkDag(DagNode *dag)
{
    return SAT_UNKNOWN;
}

void VariableGenerator::push() {}

void VariableGenerator::pop() {}

void VariableGenerator::reset() {}

void VariableGenerator::clearAssertions() {}

SmtModel *VariableGenerator::getModel() {}

void VariableGenerator::setLogic(const char *logic) {}

#endif

//
//	Common code
//

VariableDagNode *
VariableGenerator::makeFreshVariable(Term *baseVariable, const mpz_class &number)
{
    Symbol *s = baseVariable->symbol();
    VariableTerm *vt = safeCast(VariableTerm *, baseVariable);
    int id = vt->id();

    string newNameString = "#";
    char *name = mpz_get_str(0, 10, number.get_mpz_t());
    newNameString += name;
    free(name);
    newNameString += "-";
    newNameString += Token::name(id);
    int newId = Token::encode(newNameString.c_str());

    return new VariableDagNode(s, newId, NONE);
}
