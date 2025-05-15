//
//      Class for generating SMT variables, version for Python support.
//
#ifndef _py_smt_hh_
#define _py_smt_hh_

#include "Python.h"
#include "easyTerm.hh"
#include "smtManager.hh"

class PyDataConatiner {
    PyObject* data;

public:

    PyDataConatiner(PyObject* data) : data(data) { 
        Py_XINCREF(this->data); 
    };
    ~PyDataConatiner() {
        Py_XDECREF(this->data); 
    };
    PyObject* getData() { 
        Py_XINCREF(this->data); 
        return this->data;
    };
};

class PySmtTerm : public SmtTerm, public PyDataConatiner {
public:
    PySmtTerm(PyObject* data) : PyDataConatiner(data) {};
    ~PySmtTerm() {};
};

class PyTermSubst : public TermSubst {
    std::map<EasyTerm*, EasyTerm*> subst;
    
public:
    PyTermSubst() {};
    ~PyTermSubst() {
        for (auto &i : subst){
            delete i.first;
            delete i.second;
        }
    };

public:
    EasyTerm* get(EasyTerm* t){
        auto it = subst.find(t);
        if (it != subst.end()){
            return it->second;
        }
        return nullptr;
    }

    std::vector<EasyTerm*> keys(){
        std::vector<EasyTerm*> ks;
        for (auto &i : subst){
            ks.push_back(i.first);
        }
        return ks;
    };

    // aux functions
    void set(DagNode* var, DagNode* val){
        subst.insert(std::pair<EasyTerm*, EasyTerm*>(new EasyTerm(var), new EasyTerm(val)));
    }
};

class PySmtModel : public SmtModel {
public:
    PySmtModel(){};
    ~PySmtModel(){
        for (auto &i : model){
            delete i.first;
            delete i.second;
        }
    };
    void set(PyObject* k, PyObject* v){
        PySmtTerm* tk = new PySmtTerm(k);
        PySmtTerm* tv = new PySmtTerm(v);
        model.insert(std::pair<PySmtTerm*, PySmtTerm*>(tk, tv));
    }
    SmtTerm* get(SmtTerm* k){
        if (PySmtTerm* t = static_cast<PySmtTerm*>(k)){
            auto it = model.find(t);
            if (it != model.end()){
                return it->second;
            }
        }
        return nullptr;
    };
    std::vector<SmtTerm*>* keys(){
        std::vector<SmtTerm*>* ks = new std::vector<SmtTerm*>();
        for (auto &i : model){
            ks->push_back(i.first);
        }
        return ks;
    };

private:
    // std::vector<PyObject*> refs;
    std::map<PySmtTerm*, PySmtTerm*> model;
};



class PyConverter : public Converter
{
public:
	virtual ~PyConverter() {};
    virtual void prepareFor(VisibleModule* module) = 0;
    virtual PySmtTerm* pyDag2term(EasyTerm* dag) = 0;
    virtual EasyTerm* pyTerm2dag(PySmtTerm* term) = 0;

public:
    // implementation of the Converter interface
    inline SmtTerm* dag2term(DagNode* dag) {
        try {
            EasyTerm term(dag);
            return pyDag2term(&term);
        } catch (const std::exception &e) {
            PyErr_Print();
            throw std::runtime_error("Python converter error");
        }
    };

    // this converter must be pysmt term type
    inline DagNode* term2dag(SmtTerm* term){
        if (PySmtTerm* t = static_cast<PySmtTerm*>(term)){
            // this is handled by Python
            try {
                if(EasyTerm* result = pyTerm2dag(t)){
                    DagNode* dag = result->getDag();
                    delete result; // otherwise memory becomes corrupted
                    return dag;
                }
            } catch (const std::exception &e) {
                PyErr_Print();
                throw std::runtime_error("Python converter error");
            }
        }
        return nullptr;
    };
};

class PyConnector : public Connector
{
public:
	virtual ~PyConnector() {};
    virtual bool py_check_sat(std::vector<PySmtTerm*> consts) = 0;
    virtual bool py_subsume(PyTermSubst* subst, PySmtTerm* prev, PySmtTerm* acc, PySmtTerm* cur) = 0;
    // virtual bool merge(PyTermSubst* subst, PySmtTerm* prev, std::vector<PySmtTerm*> target_consts) = 0;
    virtual PySmtTerm* py_add_const(PySmtTerm* acc, PySmtTerm* cur) = 0;
    virtual PySmtModel* py_get_model() = 0;
    virtual void print_model() = 0;
    virtual void set_logic(const char* logic) = 0;
    virtual PyConverter* py_get_converter() = 0;
    virtual void push() = 0;
    virtual void pop() = 0;
    virtual void reset() = 0;

public:
    inline SmtModel* get_model(){
        return py_get_model();
    }

    inline SmtTerm* add_const(SmtTerm* acc, SmtTerm* cur){
        PySmtTerm* pyAcc = acc ? static_cast<PySmtTerm*>(acc) : nullptr;
        PySmtTerm* pyCur = cur ? static_cast<PySmtTerm*>(cur) : nullptr;
        return py_add_const(pyAcc, pyCur);
    };

    bool check_sat(std::vector<SmtTerm*> consts){
        std::vector<PySmtTerm*> pyConsts;
        for(auto &c : consts){
            PySmtTerm* con = c ? static_cast<PySmtTerm*>(c) : nullptr;
            pyConsts.push_back(con);
        }

        return py_check_sat(pyConsts);
        // return true;
    };

    inline TermSubst* mk_subst(std::map<DagNode*, DagNode*>& subst_dict){
        PyTermSubst* subst = new PyTermSubst();
        for (auto &i : subst_dict){
            subst->set(i.first, i.second);
        }
        return subst;
    };

    inline bool subsume(TermSubst* subst, SmtTerm* prev, SmtTerm* acc, SmtTerm* cur){
        if(PyTermSubst* pySubst = static_cast<PyTermSubst*>(subst)){
            PySmtTerm* pyPrev = acc ? static_cast<PySmtTerm*>(prev) : nullptr;
            PySmtTerm* pyAcc = cur ? static_cast<PySmtTerm*>(acc) : nullptr;
            PySmtTerm* pyCur = acc ? static_cast<PySmtTerm*>(cur) : nullptr;

            return py_subsume(pySubst, pyPrev, pyAcc, pyCur);
        }

        IssueWarning("substituion for subsumption checking is corrupted");
        return false;
    };

    inline Converter* get_converter(){
        return py_get_converter();
    }
};

class PySmtManagerFactory : public SmtManagerFactory
{
public:
    virtual ~PySmtManagerFactory() {};
    virtual PyConnector* py_createConnector(PyConverter* conv) = 0;
    virtual PyConverter* py_createConverter() = 0;

public:
    Connector* createConnector(Converter* conv){
        PyConverter* pyconv = dynamic_cast<PyConverter*>(conv);
        return py_createConnector(pyconv); 
    };
    Converter* createConverter(const SMT_Info& smtInfo, MetaLevelSmtOpSymbol* extensionSymbol){ return py_createConverter(); };
};

#ifdef USE_PYSMT
class SmtManagerFactorySetter : public SmtManagerFactorySetterInterface
{
public:
    void set(){};
};
#else
#endif


#endif
