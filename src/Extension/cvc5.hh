#ifndef MAUDE_SE_CVC5_EXTENSION_HH
#define MAUDE_SE_CVC5_EXTENSION_HH

#include "cvc5/cvc5.h"
#include "smtInterface.hh"
#include "nativeSmt.hh"
#include "rootedDag.hh"
#include "extGlobal.hh"
#include "simpleRootContainer.hh"
#include <map>
#include <memory>
#include <string>
#include <vector>

class Cvc5Term : public _SmtTerm
{
public:
    explicit Cvc5Term(cvc5::Term value) : value(std::move(value)) {}
    cvc5::Term value;
};

class Cvc5Substitution : public _TermSubst
{
public:
    std::vector<cvc5::Term> from;
    std::vector<cvc5::Term> to;
};

class Cvc5Model : public _SmtModel
{
public:
    explicit Cvc5Model(std::map<cvc5::Term, cvc5::Term> values)
        : values(std::move(values)) {}
    SmtTerm get(SmtTerm key) override;
    SmtTermVector keys() override;

private:
    std::map<cvc5::Term, cvc5::Term> values;
};

struct Cvc5TermLess
{
    bool operator()(const cvc5::Term &a, const cvc5::Term &b) const
    {
        return a < b;
    }
};

class Cvc5Converter : public _Converter,
                      public NativeSmtConverter<cvc5::Term, Cvc5TermLess>,
                      private SimpleRootContainer
{
public:
    explicit Cvc5Converter(const SMT_Info &info);
    void prepareFor(VisibleModule *module) override;
    SmtTerm dag2term(DagNode *dag) override;
    DagNode *term2dag(SmtTerm term) override;
    cvc5::TermManager &manager() { return tm; }
    const SmtManagerVariableMap &variables() const { return smtManagerVariableMap; }

private:
    cvc5::Term makeVariable(DagNode *dag) override;
    cvc5::Term convert(DagNode *dag);
    DagNode *convertBack(const cvc5::Term &term);
    DagNode *convertBackUnrooted(const cvc5::Term &term);
    std::vector<std::unique_ptr<RootedDag>> conversionRoots;
    void markReachableNodes() override;

    cvc5::TermManager tm;
    SymbolGetter sg;
};

class Cvc5Connector : public _Connector
{
public:
    explicit Cvc5Connector(std::shared_ptr<Cvc5Converter> converter);
    SmtResult check_sat(SmtTermVector constraints) override;
    bool subsume(TermSubst substitution, SmtTerm previous, SmtTerm accumulated,
                 SmtTerm current) override;
    TermSubst mk_subst(std::map<DagNode *, DagNode *> &substitution) override;
    SmtTerm add_const(SmtTerm accumulated, SmtTerm current) override;
    SmtModel get_model() override;
    void push() override;
    void pop() override;
    SmtTerm simplify(SmtTerm term) override;
    void print_model() override {}
    void set_logic(const char *logic) override;
    void reset() override;
    Converter get_converter() override { return conv; }

private:
    cvc5::TermManager &tm;
    std::shared_ptr<Cvc5Converter> conv;
    cvc5::Solver solver;
    std::string logic;
    unsigned pushCount = 0;
};

class Cvc5SmtManagerFactory : public SmtManagerFactory
{
public:
    Converter createConverter(const SMT_Info &info) override
    {
        return std::make_shared<Cvc5Converter>(info);
    }
    Connector createConnector(Converter converter) override
    {
        return std::make_shared<Cvc5Connector>(
            std::dynamic_pointer_cast<Cvc5Converter>(converter));
    }
};

class SmtManagerFactorySetter : public SmtManagerFactorySetterInterface
{
public:
    void set() override
    {
        delete smtManagerFactory;
        smtManagerFactory = new Cvc5SmtManagerFactory();
    }
};

#endif
