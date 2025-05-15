%module(directors="1") maudeSE

%{
#include "pysmt.hh"
#include "extGlobal.hh"
%}

%include "extGlobal.hh"

%include std_vector.i
%include std_pair.i
%include std_map.i
%include std_except.i

namespace std {
  %template (SmtTermVector) vector<SmtTerm*>;
  %template (PySmtTermVector)   vector<PySmtTerm*>;
  %template (SmtModelPair)  pair<SmtTerm*, SmtTerm*>;
	%template (SmtModelPairVector) vector<pair<SmtTerm*, SmtTerm*>>;
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

class PySmtTerm {
public:

  %rename(data) getData;

  PySmtTerm() = delete;
  PySmtTerm(PyObject* data);
  ~PySmtTerm();
  PyObject* getData();
};

class PySmtModel {
public:
    PySmtModel();
    ~PySmtModel();
    void set(PyObject* k, PyObject* v);
};

%rename (term2dag) pyTerm2dag;
%rename (dag2term) pyDag2term;

%feature("director") PyConverter;
class PyConverter
{
public:
  // %newobject pyDag2term;
  // %newobject pyTerm2dag;

  virtual ~PyConverter() {};
  virtual void prepareFor(VisibleModule* module) = 0;
  virtual PySmtTerm* pyDag2term(EasyTerm* dag) = 0;
  virtual EasyTerm* pyTerm2dag(PySmtTerm* term) = 0;
  virtual PyObject* mkApp(PyObject* symbol, PyObject* args) = 0;
  virtual PyObject* getSymbol(PyObject* dag) = 0;
};

class PyTermSubst {
public:
    PyTermSubst();
    ~PyTermSubst();
    EasyTerm* get(EasyTerm* v);
    std::vector<EasyTerm*> keys();
};

%feature("director") SmtResult;
class SmtResult{
public:
    
    virtual ~SmtResult() {};
    virtual bool is_sat() = 0;
    virtual bool is_unsat() = 0;
    virtual bool is_unknown() = 0;
};

class Connector
{
public:
	virtual ~Connector() {};
    // virtual bool check_sat(std::vector<SmtTerm*> consts) = 0;
    // virtual bool subsume(TermSubst* subst, SmtTerm* prev, SmtTerm* acc, SmtTerm* cur) = 0;
    // virtual TermSubst* mkSubst(std::vector<EasyTerm*> vars, std::vector<EasyTerm*> vals) = 0;
    // virtual PyObject* merge(PyObject* subst, PyObject* prev_const, std::vector<SmtTerm*> target_consts) = 0;
    // virtual SmtTerm* add_const(SmtTerm* acc, SmtTerm* cur) = 0;
    // virtual SmtModel* get_model() = 0;
    virtual void print_model() = 0;
    virtual void set_logic(const char* logic) = 0;
};


%rename(get_model) py_get_model;
%rename(add_const) py_add_const;
%rename(check_sat) py_check_sat;
// %rename(mk_subst) py_mk_subst;
%rename(subsume) py_subsume;
%rename(get_converter) py_get_converter;

%feature("director") PyConnector;
class PyConnector : public Connector
{
public:
  // %newobject py_check_sat;
  // %newobject py_add_const;
  // %newobject py_get_model;
  // %newobject py_mk_subst;

  virtual ~PyConnector() {};
  // virtual bool check_sat(std::vector<SmtTerm*> consts) = 0;
  virtual bool py_check_sat(std::vector<PySmtTerm*> consts) = 0;
  virtual bool py_subsume(PyTermSubst* subst, PySmtTerm* prev, PySmtTerm* acc, PySmtTerm* cur) = 0;
  // virtual PyTermSubst* py_mk_subst(std::vector<EasyTerm*> vars, std::vector<EasyTerm*> vals) = 0;
  // virtual PyObject* merge(PyObject* subst, PyObject* prev_const, std::vector<SmtTerm*> target_consts) = 0;
  virtual PySmtTerm* py_add_const(PySmtTerm* acc, PySmtTerm* cur) = 0;
  // virtual SmtTerm* add_const(SmtTerm* acc, SmtTerm* cur) = 0;
  // virtual SmtModel* get_model() = 0;
  virtual PySmtModel* py_get_model() = 0;
  virtual void print_model() = 0;
  virtual void set_logic(const char* logic) = 0;
  virtual PyConverter* py_get_converter() = 0;
  virtual void push() = 0;
  virtual void pop() = 0;
  virtual void reset() = 0;
};

%rename(createConverter) py_createConverter;
%rename(createConnector) py_createConnector;

%feature("director") PySmtManagerFactory;
class PySmtManagerFactory : public SmtManagerFactory
{
public:
  // %newobject py_createConverter;
  // %newobject py_createConnector;

  virtual ~PySmtManagerFactory() {};
  virtual PyConnector* py_createConnector(PyConverter* conv) = 0;
  virtual PyConverter* py_createConverter() = 0;
};

