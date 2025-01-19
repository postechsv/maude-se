//
//	Class for folding and maintaining the history of a search.
//
#ifndef _smt_interface_h_
#define _smt_interface_h_

#include <vector>
#include <map>

// forward decl
class EasyTerm;
class VisibleModule;
class DagNode;
class VariableGenerator;
class SMT_Info;

class SmtTerm{
public:
    virtual ~SmtTerm() {};
};

class TermSubst{
public:
    virtual ~TermSubst(){};
};

class SmtModel{
public:
    virtual ~SmtModel() {};
    virtual SmtTerm* get(SmtTerm* k) = 0;
    virtual std::vector<SmtTerm*>* keys() = 0;
};

class SmtResult{

public:
    virtual ~SmtResult() {};
    virtual bool is_sat() = 0;
    virtual bool is_unsat() = 0;
    virtual bool is_unknown() = 0;
};

class Converter
{
public:
	virtual ~Converter() {};
    virtual void prepareFor(VisibleModule* module) = 0;
    virtual SmtTerm* dag2term(DagNode* dag) = 0;
    virtual DagNode* term2dag(SmtTerm* term) = 0;
};

class Connector
{
public:
	virtual ~Connector() {};
    virtual bool check_sat(std::vector<SmtTerm*> consts) = 0;
    virtual bool subsume(TermSubst* subst, SmtTerm* prev, SmtTerm* acc, SmtTerm* cur) = 0;
    virtual TermSubst* mk_subst(std::map<DagNode*, DagNode*>& subst_dict) = 0;
    // virtual PyObject* merge(PyObject* subst, PyObject* prev_const, std::vector<SmtTerm*> target_consts) = 0;
    virtual SmtTerm* add_const(SmtTerm* acc, SmtTerm* cur) = 0;
    virtual SmtModel* get_model() = 0;
    virtual void push() = 0;
    virtual void pop() = 0;

    virtual void print_model() = 0;
    virtual void set_logic(const char* logic) = 0;
    virtual void reset() = 0;

    virtual Converter* get_converter() = 0;
};

class SmtManagerFactory
{
public:
    virtual VariableGenerator* create(const SMT_Info& smtInfo) = 0;
};

// class SmtManagerFactorySetter
// {
// public:
//     virtual void setSmtManagerFactory(SmtManagerFactory* smtManagerFactory) = 0;
// };

#endif