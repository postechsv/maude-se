//
//      Class for generating SMT variables, version for Python support.
//
#ifndef _py_smt_hh_
#define _py_smt_hh_

#include "Python.h"
#include "easyTerm.hh"
#include "nativeSmt.hh"
#include "userLevelRewritingContext.hh"
#include "smtManager.hh"

// --- Python data wrapper ---
class PyDataContainer
{
    PyObject *data;

public:
    PyDataContainer(PyObject *data) : data(data) { Py_XINCREF(this->data); }
    virtual ~PyDataContainer() { Py_XDECREF(this->data); }

    PyObject *getData()
    {
        Py_XINCREF(this->data);
        return this->data;
    }
};

// --- PySmtTerm ---
class _PySmtTerm : public _SmtTerm, public PyDataContainer
{
public:
    _PySmtTerm(PyObject *data) : PyDataContainer(data) {}
    ~_PySmtTerm() override = default;
};

using PySmtTerm = std::shared_ptr<_PySmtTerm>;

// --- PyTermSubst ---
class _PyTermSubst : public _TermSubst
{
    std::map<EasyTerm *, EasyTerm *> subst;

public:
    _PyTermSubst() = default;
    ~_PyTermSubst() override
    {
        for (auto &i : subst)
        {
            delete i.first;
            delete i.second;
        }
    }

    EasyTerm *get(EasyTerm *t)
    {
        auto it = subst.find(t);
        return (it != subst.end()) ? it->second : nullptr;
    }

    std::vector<EasyTerm *> keys()
    {
        std::vector<EasyTerm *> ks;
        for (auto &i : subst)
            ks.push_back(i.first);
        return ks;
    }

    void set(DagNode *var, DagNode *val)
    {
        subst.emplace(new EasyTerm(var), new EasyTerm(val));
    }
};

using PyTermSubst = std::shared_ptr<_PyTermSubst>;

// --- PySmtModel ---
class _PySmtModel : public _SmtModel
{
    std::shared_ptr<std::map<PySmtTerm, PySmtTerm>> model;

public:
    _PySmtModel() : model(std::make_shared<std::map<PySmtTerm, PySmtTerm>>()) {}
    ~_PySmtModel() override = default;

    void set(PyObject *k, PyObject *v)
    {
        (*model)[std::make_shared<_PySmtTerm>(k)] = std::make_shared<_PySmtTerm>(v);
    }

    SmtTerm get(SmtTerm k) override
    {
        if (!k)
            return nullptr;
        auto it = model->find(std::dynamic_pointer_cast<_PySmtTerm>(k));
        return (it != model->end()) ? it->second : nullptr;
    }

    SmtTermVector keys() override
    {
        auto ks = std::make_shared<std::vector<SmtTerm>>();
        for (const auto &p : *model)
            ks->emplace_back(p.first);
        return ks;
    }
};

using PySmtModel = std::shared_ptr<_PySmtModel>;

struct cmpExprById
{
    bool operator()(const PySmtTerm &lhs, const PySmtTerm &rhs) const
    {
        return lhs->getData() < rhs->getData();
    }
};

// --- PyConverter ---
class _PyConverter : public _Converter, private SimpleRootContainer
{
public:
    virtual ~_PyConverter() = default;

    virtual void prepareFor(VisibleModule *module) = 0;
    virtual PySmtTerm pyDag2term(EasyTerm *dag) = 0;
    virtual EasyTerm *pyTerm2dag(PySmtTerm term) = 0;

    SmtTerm dag2term(DagNode *dag) override
    {
        try
        {
            EasyTerm term(dag);
            return pyDag2term(&term); // returns PySmtTerm (aka SmtTerm)
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python dag2term error");
        }
    }

    DagNode *term2dag(SmtTerm term) override
    {
        if (PySmtTerm t = std::dynamic_pointer_cast<_PySmtTerm>(term))
        {
            // this is handled by Python
            try
            {
                if (EasyTerm *result = pyTerm2dag(t))
                {
                    DagNode *dag = result->getDag();
                    delete result; // otherwise memory becomes corrupted
                    if (dag->getSort() == nullptr)
                    {
                        RewritingContext *context = new UserLevelRewritingContext(dag);
                        dag->computeTrueSort(*context);
                        delete context;
                    }
                    return dag;
                }
            }
            catch (...)
            {
                PyErr_Print();
                throw std::runtime_error("Python term2dag error");
            }
        }
        return nullptr;
    }

    void markReachableNodes()
    {
        for (auto it = cache.begin(); it != cache.end(); it++)
        {
            it->first->mark();
        }
    }

private:
    typedef std::map<DagNode *, PySmtTerm> Cache;
    typedef std::map<PySmtTerm, DagNode *, cmpExprById> ReverseCache;

    Cache cache;
    ReverseCache rcache;

    void genRevCache()
    {
        for (auto it = cache.begin(); it != cache.end(); it++)
        {
            (rcache)[it->second] = it->first;
        }
    }

    bool python_equal(PyObject *a, PyObject *b)
    {
        Py_hash_t hash_a = PyObject_Hash(a);
        Py_hash_t hash_b = PyObject_Hash(b);

        if (hash_a == -1 || hash_b == -1)
        {
            PyErr_Print();
            return false;
        }
        return hash_a == hash_b;
    }

public:
    void cache_insert(EasyTerm *dag, PySmtTerm &term)
    {
        cache[dag->getDag()] = term;
    }

    EasyTerm *cache_find(PySmtTerm &term)
    {

        genRevCache();

        PyObject *pyObj = term->getData();
        for (auto &[key, val] : rcache)
        {
            if (python_equal(key->getData(), pyObj))
            {
                return new EasyTerm(val);
            }
        }

        return nullptr;
    }

    PySmtTerm cache_find(EasyTerm *dag)
    {
        DagNode *d = dag->getDag();
        auto it = cache.find(d);
        if (it != cache.end())
            return it->second;

        for (auto it2 = cache.begin(); it2 != cache.end(); ++it2)
        {
            if (d->equal(it2->first))
            {
                cache.insert({d, it2->second});
                return it2->second;
            }
        }
        return nullptr;
    }
};

using PyConverter = std::shared_ptr<_PyConverter>;
using PySmtTermVector = std::shared_ptr<std::vector<PySmtTerm>>;

// --- PyConnector ---
class _PyConnector : public _Connector
{
public:
    virtual ~_PyConnector() = default;

    virtual SmtResult py_check_sat(std::vector<PySmtTerm> &consts) = 0;
    virtual bool py_subsume(PyTermSubst subst, PySmtTerm prev, PySmtTerm acc, PySmtTerm cur) = 0;
    virtual PySmtTerm py_add_const(PySmtTerm acc, PySmtTerm cur) = 0;
    virtual PySmtModel py_get_model() = 0;
    virtual PyConverter py_get_converter() = 0;
    virtual PySmtTerm py_simplify(PySmtTerm term) = 0;
    virtual void py_set_logic(const char *logic) = 0;

    SmtResult check_sat(SmtTermVector consts) override
    {
        if (!consts)
            throw std::invalid_argument("check_sat received null SmtTermVector");

        std::vector<PySmtTerm> pyConsts;
        for (auto &c : *consts)
            pyConsts.push_back(std::dynamic_pointer_cast<_PySmtTerm>(c));

        try
        {
            return py_check_sat(pyConsts);
        }
        catch (...)
        {
            // handle error as unknown case
            PyErr_Clear();
            return unknown;
        }
    }

    SmtTerm add_const(SmtTerm acc, SmtTerm cur) override
    {
        auto pyAcc = std::dynamic_pointer_cast<_PySmtTerm>(acc);
        auto pyCur = std::dynamic_pointer_cast<_PySmtTerm>(cur);
        try
        {
            return py_add_const(pyAcc, pyCur);
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python add_const error");
        }
    }

    SmtModel get_model() override
    {
        try
        {
            return py_get_model();
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python get_model error");
        }
    }

    TermSubst mk_subst(std::map<DagNode *, DagNode *> &subst_dict) override
    {
        auto subst = std::make_shared<_PyTermSubst>();
        for (auto &[k, v] : subst_dict)
            subst->set(k, v);
        return subst;
    }

    bool subsume(TermSubst subst, SmtTerm prev, SmtTerm acc, SmtTerm cur) override
    {
        try
        {
            return py_subsume(dynamic_pointer_cast<_PyTermSubst>(subst), dynamic_pointer_cast<_PySmtTerm>(prev),
                              dynamic_pointer_cast<_PySmtTerm>(acc), dynamic_pointer_cast<_PySmtTerm>(cur));
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python subsume error");
        }
    }

    Converter get_converter() override
    {
        try
        {

            return py_get_converter();
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python get_converter error");
        }
    }

    SmtTerm simplify(SmtTerm term)
    {
        try
        {
            return py_simplify(dynamic_pointer_cast<_PySmtTerm>(term));
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python simplify error");
        }
    }

    void set_logic (const char *logic)
    {
        try
        {
            return py_set_logic(logic);
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python set_logic error");
        }
    }

    virtual void push() override = 0;
    virtual void pop() override = 0;
    virtual void reset() override = 0;
    virtual void print_model() override = 0;
};
using PyConnector = std::shared_ptr<_PyConnector>;

// --- PySmtManagerFactory ---
class PySmtManagerFactory : public SmtManagerFactory
{
public:
    virtual ~PySmtManagerFactory() = default;

    virtual PyConnector py_createConnector(PyConverter conv) = 0;
    virtual PyConverter py_createConverter() = 0;

    Connector createConnector(Converter conv) override
    {
        try
        {
            return py_createConnector(std::dynamic_pointer_cast<_PyConverter>(conv));
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python createConnector error");
        }
    }

    Converter createConverter(const SMT_Info &) override
    {
        try
        {
            return py_createConverter();
        }
        catch (...)
        {
            PyErr_Print();
            throw std::runtime_error("Python createConverter error");
        }
    }
};

#ifdef USE_PYSMT
class SmtManagerFactorySetter : public SmtManagerFactorySetterInterface
{
public:
    void set() {};
};
#else
#endif

#endif // _pysmt_hh_
