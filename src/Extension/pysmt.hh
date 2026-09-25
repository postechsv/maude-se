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
#include "rootedDag.hh"
#include <unordered_map>

// --- Python data wrapper ---
class PyDataContainer
{
    PyObject *data;

public:
    PyDataContainer(PyObject *data) : data(data)
    {
        if (Py_IsInitialized())
        {
            PyGILState_STATE gil = PyGILState_Ensure();
            Py_XINCREF(this->data);
            PyGILState_Release(gil);
        }
    }
    virtual ~PyDataContainer()
    {
        if (Py_IsInitialized())
        {
            PyGILState_STATE gil = PyGILState_Ensure();
            Py_XDECREF(this->data);
            PyGILState_Release(gil);
        }
    }

    // Borrowed reference: only use while the owning term and GIL are held.
    PyObject *borrowData() const { return data; }

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
        // SWIG owns the wrappers returned by keys(), so compare terms rather
        // than the addresses of those temporary wrappers.
        for (auto &entry : subst)
            if (entry.first->equal(t))
                return entry.second;
        return nullptr;
    }

    std::vector<EasyTerm *> keys()
    {
        std::vector<EasyTerm *> ks;
        for (auto &i : subst)
            ks.push_back(new EasyTerm(i.first->getDag()));
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
        return std::less<PyObject *>()(lhs->borrowData(), rhs->borrowData());
    }
};

// --- PyConverter ---
class _PyConverter : public _Converter, private SimpleRootContainer
{
public:
    virtual ~_PyConverter() = default;

    virtual void py_prepareFor(VisibleModule *module) = 0;
    virtual PySmtTerm pyDag2term(EasyTerm *dag) = 0;
    virtual EasyTerm *pyTerm2dag(PySmtTerm term) = 0;

    void prepareFor(VisibleModule *module) override
    {
        conversionCache.clear();
        conversionCacheSize = 0;
        rcache.clear();
        cache.clear();
        py_prepareFor(module);
    }

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

    DagHandle term2dag(SmtTerm term) override
    {
        if (PySmtTerm t = std::dynamic_pointer_cast<_PySmtTerm>(term))
        {
            // this is handled by Python
            try
            {
                if (EasyTerm *result = pyTerm2dag(t))
                {
                    DagNode *dag = result->getDag();
                    DagHandle resultRoot(dag);
                    delete result; // otherwise memory becomes corrupted
                    if (dag->getSort() == nullptr)
                    {
                        RewritingContext *context = new UserLevelRewritingContext(dag);
                        dag->computeTrueSort(*context);
                        delete context;
                    }
                    return resultRoot;
                }
            }
            catch (...)
            {
                PyErr_Print();
                throw std::runtime_error("Python term2dag error");
            }
        }
        return {};
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

    struct ConversionCacheEntry
    {
        DagHandle dag;
        PySmtTerm term;
    };

    std::unordered_map<size_t, std::vector<ConversionCacheEntry>> conversionCache;
    size_t conversionCacheSize = 0;

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
        if (a == b)
            return true;

        // Z3's __eq__ constructs a symbolic formula rather than a Python
        // boolean. Its eq() method compares AST identities and returns bool.
        PyObject *eqMethod = PyObject_GetAttrString(a, "eq");
        if (eqMethod)
        {
            if (PyCallable_Check(eqMethod))
            {
                PyObject *answer = PyObject_CallFunctionObjArgs(eqMethod, b, nullptr);
                if (answer)
                {
                    if (PyBool_Check(answer))
                    {
                        bool equal = answer == Py_True;
                        Py_DECREF(answer);
                        Py_DECREF(eqMethod);
                        return equal;
                    }
                    Py_DECREF(answer);
                }
                else
                    PyErr_Clear();
            }
            Py_DECREF(eqMethod);
        }
        else
            PyErr_Clear();

        int equal = PyObject_RichCompareBool(a, b, Py_EQ);
        if (equal < 0)
        {
            // Some solver objects expose only a symbolic comparison.
            PyErr_Clear();
            return false;
        }
        return equal == 1;
    }

public:
    // Unlike the variable cache below, this cache also stores compound terms.
    // DagHandle keeps each key visible to Maude's collector.
    PySmtTerm conversion_cache_find(EasyTerm *term)
    {
        DagNode *dag = term->getDag();
        auto bucket = conversionCache.find(dag->getHashValue());
        if (bucket != conversionCache.end())
            for (const auto &entry : bucket->second)
                if (dag == entry.dag.get() || dag->equal(entry.dag.get()))
                    return entry.term;
        return nullptr;
    }

    void conversion_cache_insert(EasyTerm *term, PySmtTerm value)
    {
        if (!value) return;
        // Bound retained DAGs and solver objects for long-running searches.
        if (conversionCacheSize >= 4096)
        {
            conversionCache.clear();
            conversionCacheSize = 0;
        }
        DagNode *dag = term->getDag();
        conversionCache[dag->getHashValue()].push_back({DagHandle(dag), value});
        ++conversionCacheSize;
    }

    void cache_insert(EasyTerm *dag, PySmtTerm &term)
    {
        cache[dag->getDag()] = term;
    }

    EasyTerm *cache_find(PySmtTerm &term)
    {

        genRevCache();

        PyObject *pyObj = term->borrowData();
        for (auto &[key, val] : rcache)
        {
            if (python_equal(key->borrowData(), pyObj))
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

#ifdef USE_PYSMT
class SmtManagerFactorySetter : public SmtManagerFactorySetterInterface
{
public:
    void set() {};
};
#else
#endif

#endif // _pysmt_hh_
