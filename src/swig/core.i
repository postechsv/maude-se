%module(directors="1") maudeSE

%{
#include "smt_wrapper.hh"
%}

%include std_vector.i
%include std_pair.i
%include std_map.i

namespace std {
  %template (SmtTermVector) vector<SmtTerm*>;
  // %template (PySmtModelMap)   map<PySmtTerm*, PySmtTerm*>;
  %template (PySmtTermVector)   vector<PySmtTerm*>;
  // %template (ModelDict)     map<PyObject*, PyObject*>;
  %template (SmtModelPair)  pair<SmtTerm*, SmtTerm*>;
	%template (SmtModelPairVector) vector<pair<SmtTerm*, SmtTerm*>>;
}



// %template
// %template (PySmtModel) Model<PySmtTerm*>;
// %rename (SmtModel) PySmtModel;

// class PySmtTerm {

// public:
//   PySmtTerm(PyObject*);
//   ~PySmtTerm();

// %extend {
//   %pythoncode{
//     def getData(self):
//       return self.data
//   }

// }
// };

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


class Converter
{
public:
  // %newobject dag2term;
  // %newobject dag2term;

	virtual ~Converter() {};
  virtual PyObject* prepareFor(VisibleModule* module) = 0;
  // virtual SmtTerm* dag2term(DagNode* dag) = 0;
  // virtual DagNode* term2dag(SmtTerm* term) = 0;
  virtual PyObject* mkApp(PyObject* symbol, PyObject* args) = 0;
  virtual PyObject* getSymbol(PyObject* dag) = 0;
};

%rename (term2dag) pyTerm2dag;
%rename (dag2term) pyDag2term;

%feature("director") PyConverter;
class PyConverter : public Converter
{
public:
  %newobject pyDag2term;
  %newobject pyTerm2dag;

  virtual ~PyConverter() {};
  virtual PyObject* prepareFor(VisibleModule* module) = 0;
  virtual PySmtTerm* pyDag2term(EasyTerm* dag) = 0;
  virtual EasyTerm* pyTerm2dag(PySmtTerm* term) = 0;
  virtual PyObject* mkApp(PyObject* symbol, PyObject* args) = 0;
  virtual PyObject* getSymbol(PyObject* dag) = 0;
};

// class SmtTerm{
// public:
//   SmtTerm() = delete;
//   SmtTerm(PyObject* data);
//   ~SmtTerm();
//   PyObject* getData();
// };

// class PyTermSubst {
// public:

//   %rename(subst) getData;

//   PyTermSubst() = delete;
//   PyTermSubst(PyObject* data);
//   ~PyTermSubst();
//   PyObject* getData();
// };

class PyTermSubst {
public:
    PyTermSubst();
    ~PyTermSubst();
    EasyTerm* get(EasyTerm* v);
    std::vector<EasyTerm*> keys();
};

// class SmtModel{
// public:
//     PyObject* model;

//     SmtModel() = delete;
//     SmtModel(PyObject* model);
//     PyObject* getModel();
// };

// class SmtModel{
// public:
//     %newobject keys;

//     SmtModel();
//     ~SmtModel();
//     void set(SmtTerm* k, SmtTerm* v);
//     SmtTerm* get(SmtTerm* k);
//     std::vector<SmtTerm*>* keys();
// };

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

%feature("director") PyConnector;
class PyConnector : public Connector
{
public:
  %newobject py_check_sat;
  %newobject py_add_const;
  %newobject py_get_model;
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
};

%rename(createConverter) py_createConverter;
%rename(createConnector) py_createConnector;

// class WrapperFactory
// {
// public:
//     %newobject py_createConverter;
//     %newobject py_createConnector;

//     virtual ~WrapperFactory() {};
//     virtual Converter* py_createConverter() = 0;
//     virtual Connector* py_createConnector() = 0;
// };

%feature("director") PyWrapperFactory;
class PyWrapperFactory
{
public:
  %newobject py_createConverter;
  %newobject py_createConnector;

  virtual ~PyWrapperFactory() {};
  virtual Converter* py_createConverter() = 0;
  virtual Connector* py_createConnector() = 0;
};