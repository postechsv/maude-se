%module(directors="1") maudeSE

%{
#include "pysmt.hh"
#include "smtInterface.hh"
#include "extGlobal.hh"
%}

%include "extGlobal.hh"
%include <std_shared_ptr.i>
%include <std_vector.i>
%include <std_except.i>

// ------------------------
// shared_ptr registrations
// ------------------------
%shared_ptr(_PySmtTerm)
%shared_ptr(_PyTermSubst)
%shared_ptr(_PySmtModel)
%shared_ptr(_PyConverter)
%shared_ptr(_PyConnector)

// ------------------------
// vector registrations
// ------------------------
namespace std {
  %template(_PySmtTermVector) vector<shared_ptr<_PySmtTerm>>;
}
// %shared_ptr(_PySmtTermVector)

// ------------------------
// Exception handling
// ------------------------
%feature("director:except") {
    if ($error != NULL) {
        throw Swig::DirectorMethodException();
    }
}

%exception {
    try { $action }
    catch (Swig::DirectorException &e) { SWIG_fail; }
}

// ------------------------
// SWIG Renames (internal types only)
// ------------------------
%rename(SmtTerm) _PySmtTerm;
%ignore PyDataContainer;
%rename(SmtModel) _PySmtModel;
%rename(TermSubst) _PyTermSubst;

// --- Converter ---
%feature("director") _PyConverter;
%rename(Converter) _PyConverter;
%rename(prepareFor) py_prepareFor;
%rename(dag2term) pyDag2term;
%rename(term2dag) pyTerm2dag;
%newobject _PyConverter::cache_find;
%ignore markReachableNodes;
%ignore _PyConverter::prepareFor;
%ignore _PyConverter::term2dag;

// --- Connector ---
%feature("director") _PyConnector;
%rename(Connector) _PyConnector;
%rename(get_model) py_get_model;
%rename(add_const) py_add_const;
%rename(check_sat) py_check_sat;
%rename(subsume) py_subsume;
%rename(get_converter) py_get_converter;
%rename(simplify) py_simplify;
%rename(set_logic) py_set_logic;

// ------------------------
// Include C++ interface
// ------------------------
%include "pysmt.hh"
%include "smtConst.hh"

%inline %{
PyObject* get_data(PyObject* obj)
{
    std::shared_ptr<_PySmtTerm> *ptr = 0;
    int res = SWIG_ConvertPtr(obj, (void**)&ptr, SWIGTYPE_p_std__shared_ptrT__PySmtTerm_t, 0);
    if (!SWIG_IsOK(res) || !ptr || !(*ptr)) {
        PyErr_SetString(PyExc_TypeError, "Expected SmtTerm (PySmtTerm)");
        return nullptr;
    }
    auto term = *ptr;
    PyObject* data = term->getData();
    if (!data) {
        PyErr_SetString(PyExc_ValueError, "internal data is null");
        return nullptr;
    }
    return data;
}

// A factory owned by the C++ side. Each returned shared_ptr retains its
// Python director proxy and releases it only after the last C++ user is gone.
// This avoids disowning a SWIG shared_ptr (which strands its owner).
class OwnedPythonSmtFactory : public SmtManagerFactory
{
public:
    OwnedPythonSmtFactory(PyObject *converterClass, PyObject *connectorClass)
        : converterClass(converterClass), connectorClass(connectorClass)
    {
        Py_INCREF(converterClass);
        Py_INCREF(connectorClass);
    }

    ~OwnedPythonSmtFactory() override
    {
        if (Py_IsInitialized())
        {
            PyGILState_STATE gil = PyGILState_Ensure();
            Py_DECREF(converterClass);
            Py_DECREF(connectorClass);
            PyGILState_Release(gil);
        }
    }

    Converter createConverter(const SMT_Info &) override
    {
        PyGILState_STATE gil = PyGILState_Ensure();
        PyObject *proxy = PyObject_CallNoArgs(converterClass);
        if (!proxy)
        {
            PyErr_Print();
            PyGILState_Release(gil);
            throw std::runtime_error("Python converter construction failed");
        }
        void *pointer = nullptr;
        int result = SWIG_ConvertPtr(proxy, &pointer,
            SWIGTYPE_p_std__shared_ptrT__PyConverter_t, 0);
        if (!SWIG_IsOK(result) || !pointer || !(*static_cast<PyConverter *>(pointer)))
        {
            Py_DECREF(proxy);
            PyGILState_Release(gil);
            throw std::runtime_error("Python converter has an invalid SWIG type");
        }
        auto original = *static_cast<PyConverter *>(pointer);
        _PyConverter *raw = original.get();
        converterProxies[raw] = proxy;
        Converter owned(raw, [this, proxy](_Converter *converter) {
            if (!Py_IsInitialized()) return;
            PyGILState_STATE gil = PyGILState_Ensure();
            converterProxies.erase(static_cast<_PyConverter *>(converter));
            Py_DECREF(proxy);
            PyGILState_Release(gil);
        });
        PyGILState_Release(gil);
        return owned;
    }

    Connector createConnector(Converter converter) override
    {
        PyGILState_STATE gil = PyGILState_Ensure();
        auto *rawConverter = dynamic_cast<_PyConverter *>(converter.get());
        auto found = converterProxies.find(rawConverter);
        if (found == converterProxies.end())
        {
            PyGILState_Release(gil);
            throw std::runtime_error("Python connector has no live converter");
        }
        PyObject *proxy = PyObject_CallFunctionObjArgs(connectorClass, found->second, nullptr);
        if (!proxy)
        {
            PyErr_Print();
            PyGILState_Release(gil);
            throw std::runtime_error("Python connector construction failed");
        }
        void *pointer = nullptr;
        int result = SWIG_ConvertPtr(proxy, &pointer,
            SWIGTYPE_p_std__shared_ptrT__PyConnector_t, 0);
        if (!SWIG_IsOK(result) || !pointer || !(*static_cast<PyConnector *>(pointer)))
        {
            Py_DECREF(proxy);
            PyGILState_Release(gil);
            throw std::runtime_error("Python connector has an invalid SWIG type");
        }
        auto original = *static_cast<PyConnector *>(pointer);
        _PyConnector *raw = original.get();
        ++activeConnectors;
        Connector owned(raw, [this, proxy](_Connector *) {
            if (!Py_IsInitialized()) return;
            PyGILState_STATE gil = PyGILState_Ensure();
            Py_DECREF(proxy);
            --activeConnectors;
            PyGILState_Release(gil);
        });
        PyGILState_Release(gil);
        return owned;
    }

    bool unused() const { return converterProxies.empty() && activeConnectors == 0; }

private:
    PyObject *converterClass;
    PyObject *connectorClass;
    std::map<_PyConverter *, PyObject *> converterProxies;
    size_t activeConnectors = 0;
};

void install_python_smt_factory(PyObject *converterClass, PyObject *connectorClass)
{
    if (!PyCallable_Check(converterClass) || !PyCallable_Check(connectorClass))
        throw std::invalid_argument("Python SMT backends must be callable classes");
    // Existing searches may still own an earlier factory's adapters.
    static std::vector<std::unique_ptr<OwnedPythonSmtFactory>> factories;
    factories.push_back(std::make_unique<OwnedPythonSmtFactory>(converterClass, connectorClass));
    setSmtManagerFactory(factories.back().get());
    for (auto it = factories.begin(); it != factories.end() - 1;)
        it = (*it)->unused() ? factories.erase(it) : ++it;
}
%}
