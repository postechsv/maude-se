// utility stuff
#include "macros.hh"
#include "vector.hh"

// forward declarations
#include "strategyLanguage.hh"
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "mixfix.hh"
#include "higher.hh"
#include "SMT.hh"

#include "module.hh"
#include "interpreter.hh"
#include "global.hh"
#include "syntacticPreModule.hh"
#include "visibleModule.hh"
#include "userLevelRewritingContext.hh"

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
#include "extGlobal.hh"

//	front end class definitions
#include "token.hh"

VisibleModule *getCurrentModule()
{
    SyntacticPreModule *premodule = interpreter.getCurrentModule();

    if (premodule == nullptr || premodule->getFlatSignature()->isBad())
        return nullptr;

    VisibleModule *vmod = premodule->getFlatModule();

    if (vmod->isBad())
        return nullptr;

    vmod->protect();
    return vmod;
}

VariableGenerator::VariableGenerator(const SMT_Info &smtInfo, bool use_cur_module, bool use_folding_check) : conn(nullptr), conn2(nullptr), conv(nullptr)
{
    SmtManagerFactorySetter *smfs = new SmtManagerFactorySetter();
    smfs->set();
    delete smfs;

    conv = smtManagerFactory->createConverter(smtInfo);
    conn = smtManagerFactory->createConnector(conv);

    if (use_folding_check)
        conn2 = smtManagerFactory->createConnector(conv);

    if (use_cur_module)
        conv->prepareFor(getCurrentModule());
}

VariableGenerator::~VariableGenerator()
{
    if (conn)
        delete conn;
    if (conn2)
        delete conn2;
    if (conv)
        delete conv;
}

VariableGenerator::Result
VariableGenerator::assertDag(DagNode *dag)
{
    SmtTerm *o = conv->dag2term(dag);
    std::vector<SmtTerm *> formulas;
    formulas.push_back(o);

    if (conn->check_sat(formulas))
    {
        return SAT;
    }
    return UNSAT;
}

VariableGenerator::Result
VariableGenerator::checkDag(DagNode *dag)
{
    push();
    Result r = assertDag(dag);
    pop();

    return r;
}

inline void
VariableGenerator::clearAssertions() { conn->reset(); }

inline void
VariableGenerator::push() { conn->push(); }

inline void
VariableGenerator::pop() { conn->pop(); }

SmtModel *VariableGenerator::getModel()
{
    return conn->get_model();
}

inline void
VariableGenerator::setLogic(const char *logic)
{
    conn->set_logic(logic);
}

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

bool containsSpecialChars(const char *str)
{
    if (str != nullptr)
        for (char last = 0; *str != '\0'; last = *str, str++)
            if (Token::specialChar(*str) && last != '`')
                return true;

    return false;
}

string escapeWithBackquotes(const char *str)
{
    string escaped;

    // Add backquotes before special characters if not already there
    for (char last = 0; *str != '\0'; last = *str, str++)
    {
        if (Token::specialChar(*str) && last != '`')
            escaped.push_back('`');
        escaped.push_back(*str);
    }

    return escaped;
}

int encodeEscapedToken(const char *str)
{
    // Escape the string only if it is needed
    if (!containsSpecialChars(str))
        return Token::encode(str);

    string escaped = escapeWithBackquotes(str);
    return Token::encode(escaped.c_str());
}

#ifdef USE_CVC4
#include "cvc4.cc"
#elif defined(USE_YICES2)
#include "yices2.cc"
#elif defined(USE_Z3)
#include "z3.cc"
#elif defined(USE_PYSMT)
#else

// dummy

SmtTerm *DummyConverter::dag2term(DagNode *dag)
{
    IssueWarning("No SMT solver connected at compile time.");
    return nullptr;
}

DagNode *DummyConverter::term2dag(SmtTerm *term)
{
    IssueWarning("No SMT solver connected at compile time.");
    return nullptr;
}

bool DummyConnector::check_sat(std::vector<SmtTerm *> consts)
{
    IssueWarning("No SMT solver connected at compile time.");
    return false;
}

bool DummyConnector::subsume(TermSubst *subst, SmtTerm *prev, SmtTerm *acc, SmtTerm *cur)
{
    IssueWarning("No SMT solver connected at compile time.");
    return false;
}

TermSubst *DummyConnector::mk_subst(std::map<DagNode *, DagNode *> &subst_dict)
{
    IssueWarning("No SMT solver connected at compile time.");
    return nullptr;
}

SmtTerm *DummyConnector::add_const(SmtTerm *acc, SmtTerm *cur)
{
    IssueWarning("No SMT solver connected at compile time.");
    return nullptr;
}

SmtModel *DummyConnector::get_model()
{
    IssueWarning("No SMT solver connected at compile time.");
    return nullptr;
}

VariableGenerator *DummySmtManagerFactory::create(const SMT_Info &smtInfo)
{
    DummyConnector *dummyConnector = new DummyConnector(new DummyConverter());
    VariableGenerator *vg = new VariableGenerator(smtInfo, dummyConnector);
    return vg;
}

#endif