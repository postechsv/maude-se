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
%shared_ptr(_PySmtManagerFactory)

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
%rename(dag2term) pyDag2term;
%rename(term2dag) pyTerm2dag;
%ignore markReachableNodes;

// --- Connector ---
%feature("director") _PyConnector;
%rename(Connector) _PyConnector;
%rename(get_model) py_get_model;
%rename(add_const) py_add_const;
%rename(check_sat) py_check_sat;
%rename(subsume) py_subsume;
%rename(get_converter) py_get_converter;
%rename(simplify) py_simplify;

// --- ManagerFactory ---
%feature("director") PySmtManagerFactory;
%rename(SmtManagerFactory) PySmtManagerFactory;
%rename(createConverter) py_createConverter;
%rename(createConnector) py_createConnector;

// ------------------------
// Include C++ interface
// ------------------------
%include "pysmt.hh"

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
%}