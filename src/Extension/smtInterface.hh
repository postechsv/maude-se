//
//	Class for folding and maintaining the history of a search.
//
#ifndef _smt_interface_h_
#define _smt_interface_h_

#include <vector>
#include <map>
#include <memory>
#include "smtConst.hh"

// forward decl
class EasyTerm;
class VisibleModule;
class DagNode;
class VariableGenerator;
class SMT_Info;

class _SmtTerm
{
public:
    virtual ~_SmtTerm() {};
};

class _TermSubst
{
public:
    virtual ~_TermSubst() {};
};

using SmtTerm = std::shared_ptr<_SmtTerm>;
using TermSubst = std::shared_ptr<_TermSubst>;
using SmtTermVector = std::shared_ptr<std::vector<SmtTerm>>;

class _SmtModel
{
public:
    virtual ~_SmtModel() {};
    virtual SmtTerm get(SmtTerm k) = 0;
    virtual SmtTermVector keys() = 0;
};

using SmtModel = std::shared_ptr<_SmtModel>;

class _Converter
{
public:
    virtual ~_Converter() {};
    virtual void prepareFor(VisibleModule *module) = 0;
    virtual SmtTerm dag2term(DagNode *dag) = 0;
    virtual DagNode *term2dag(SmtTerm term) = 0;
};

using Converter = std::shared_ptr<_Converter>;

class _Connector
{
public:
    virtual ~_Connector() {};
    virtual SmtResult check_sat(SmtTermVector consts) = 0;
    virtual bool subsume(TermSubst subst, SmtTerm prev, SmtTerm acc, SmtTerm cur) = 0;
    virtual TermSubst mk_subst(std::map<DagNode *, DagNode *> &subst_dict) = 0;
    // virtual PyObject* merge(PyObject* subst, PyObject* prev_const, std::vector<SmtTerm*> target_consts) = 0;
    virtual SmtTerm add_const(SmtTerm acc, SmtTerm cur) = 0;
    virtual SmtModel get_model() = 0;
    virtual void push() = 0;
    virtual void pop() = 0;
    virtual SmtTerm simplify(SmtTerm term) = 0;

    virtual void print_model() = 0;
    virtual void set_logic(const char *logic) = 0;
    virtual void reset() = 0;

    virtual Converter get_converter() = 0;
};

using Connector = std::shared_ptr<_Connector>;

class SmtManagerFactory
{
public:
    virtual Connector createConnector(Converter conv) = 0;
    virtual Converter createConverter(const SMT_Info &smtInfo) = 0;
};

class SmtManagerFactorySetterInterface
{
public:
    virtual void set() = 0;
};

#endif