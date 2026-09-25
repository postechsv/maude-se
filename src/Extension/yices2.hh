#ifndef MAUDE_SE_YICES2_EXTENSION_HH
#define MAUDE_SE_YICES2_EXTENSION_HH

#include <gmp.h>
#include <yices.h>
#include "smtInterface.hh"
#include "nativeSmt.hh"
#include "extGlobal.hh"
#include "simpleRootContainer.hh"
#include "rootedDag.hh"
#include <map>
#include <memory>
#include <string>

class YicesTerm : public _SmtTerm
{
public:
    YicesTerm(term_t value, type_t type = NULL_TYPE, DagNode *original = nullptr)
        : value(value), type(type), original(original ? std::make_shared<RootedDag>(original) : nullptr) {}
    term_t value;
    type_t type;
    std::shared_ptr<RootedDag> original;
};

class YicesSubstitution : public _TermSubst
{
public:
    std::vector<term_t> from;
    std::vector<term_t> to;
};

class YicesModel : public _SmtModel
{
public:
    explicit YicesModel(std::map<term_t, std::pair<term_t, type_t>> values)
        : values(std::move(values)) {}
    SmtTerm get(SmtTerm key) override;
    SmtTermVector keys() override;

private:
    std::map<term_t, std::pair<term_t, type_t>> values;
};

class YicesConverter : public _Converter,
                       public NativeSmtConverter<term_t, std::less<term_t>>,
                       private SimpleRootContainer
{
public:
    explicit YicesConverter(const SMT_Info &info);
    void prepareFor(VisibleModule *module) override;
    SmtTerm dag2term(DagNode *dag) override;
    DagHandle term2dag(SmtTerm term) override;
    const SmtManagerVariableMap &variables() const { return smtManagerVariableMap; }

private:
    term_t makeVariable(DagNode *dag) override;
    term_t convert(DagNode *dag);
    DagNode *convertBack(term_t value, type_t expectedType);
    DagNode *remember(term_t value, DagNode *dag);
    void markReachableNodes() override;
    SymbolGetter sg;
    std::map<term_t, std::shared_ptr<RootedDag>> reverseCache;
};

class YicesConnector : public _Connector
{
public:
    explicit YicesConnector(std::shared_ptr<YicesConverter> converter);
    ~YicesConnector() override;
    SmtResult check_sat(SmtTermVector constraints) override;
    bool subsume(TermSubst substitution, SmtTerm previous, SmtTerm accumulated,
                 SmtTerm current) override;
    TermSubst mk_subst(std::map<DagNode *, DagNode *> &substitution) override;
    SmtTerm add_const(SmtTerm accumulated, SmtTerm current) override;
    SmtModel get_model() override;
    void push() override;
    void pop() override;
    SmtTerm simplify(SmtTerm term) override { return term; }
    void print_model() override {}
    void set_logic(const char *logic) override;
    void reset() override;
    Converter get_converter() override { return conv; }

private:
    std::shared_ptr<YicesConverter> conv;
    context_t *context;
    std::string logic;
    unsigned pushCount = 0;
};

class YicesSmtManagerFactory : public SmtManagerFactory
{
public:
    Converter createConverter(const SMT_Info &info) override
    {
        return std::make_shared<YicesConverter>(info);
    }
    Connector createConnector(Converter converter) override
    {
        return std::make_shared<YicesConnector>(
            std::dynamic_pointer_cast<YicesConverter>(converter));
    }
};

class SmtManagerFactorySetter : public SmtManagerFactorySetterInterface
{
public:
    void set() override
    {
        delete smtManagerFactory;
        smtManagerFactory = new YicesSmtManagerFactory();
    }
};

#endif
