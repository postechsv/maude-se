%module(directors="1") maudeSE

%{
#include "pysmt.hh"
#include "extGlobal.hh"
%}

%include "extGlobal.hh"

%include std_vector.i
%include std_except.i

namespace std {
  %template (SmtTermVector) vector<PySmtTerm*>;
}

%feature("director:except") {
    if ($error != NULL) {
        throw Swig::DirectorMethodException();
    }
}

%exception {
    try { $action }
    catch (Swig::DirectorException &e) { SWIG_fail; }
}

// No %newobject should be used
// because we want to give the ownership to the C++-side

// Data classes
%rename(SmtTerm) PySmtTerm;
%rename(data) getData;

%rename(SmtModel) PySmtModel;
%rename(TermSubst) PyTermSubst;

%feature("director") SmtResult;

// Converter & Connector
%feature("director") PyConverter;
%rename(Converter) PyConverter;
%rename(term2dag) pyTerm2dag;
%rename(dag2term) pyDag2term;

%feature("director") PyConnector;
%rename(Connector) PyConnector;
%rename(get_model) py_get_model;
%rename(add_const) py_add_const;
%rename(check_sat) py_check_sat;
%rename(subsume) py_subsume;
%rename(get_converter) py_get_converter;

// ManagerFactory
%feature("director") PySmtManagerFactory;
%rename(SmtManagerFactory) PySmtManagerFactory;
%rename(createConverter) py_createConverter;
%rename(createConnector) py_createConnector;

%include "pysmt.hh"