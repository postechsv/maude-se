#ifndef SMT_MANAGER_HH
#define SMT_MANAGER_HH
#include "smtEngineWrapperEx.hh"
#include "smtInterface.hh"
#include "token.hh"
#include "visibleModule.hh"

class VariableGenerator : public SmtEngineWrapperEx
{
    NO_COPYING(VariableGenerator);

public:
    VariableGenerator(const SMT_Info &smtInfo, bool use_cur_module = true, bool use_folding_check = false);
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
    inline Converter *getConverter() { return conv; };
    inline Connector *getConnector() { return conn; };
    inline Connector *getConnector2() { return conn2; }; // for folding

private:
    Connector *conn;
    Converter *conv;
    Connector *conn2; // for folding check
};

// Common auxiliary class and functions for dag generation.
// Taken from Maude as a library.

bool containsSpecialChars(const char *str);
string escapeWithBackquotes(const char *str);
int encodeEscapedToken(const char *str);

class SymbolGetter
{
public:
    inline void setModule(VisibleModule *module) { this->module = module; };

    // Auxiliary functions
    inline Symbol *getSymbol(const char *name, Vector<ConnectedComponent *> &domain, ConnectedComponent *range)
    {
        checkModule();
        if (Symbol *s = this->module->findSymbol(encodeEscapedToken(name), domain, range))
            return s;
        throw std::runtime_error("failed to find a target symbol");
    }

    inline Sort *getSort(const char *s)
    {
        checkModule();
        return this->module->findSort(encodeEscapedToken(s));
    }

    inline ConnectedComponent *getKind(const char *s)
    {
        if (Sort *sort = getSort(s))
            return sort->component();
        throw std::runtime_error("failed to find a target sort");
    }

private:
    inline void checkModule()
    {
        if (!this->module)
            std::runtime_error("invalid module given");
    }

private:
    VisibleModule *module;
};

VisibleModule *getCurrentModule();

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
