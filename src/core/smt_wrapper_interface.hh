//
//	Class for folding and maintaining the history of a search.
//
#ifndef _interface_h_
#define _interface_h_

#include "Python.h"
#include <vector>

// forward decl
class EasyTerm;
class VisibleModule;
class DagNode;

class SmtTerm{
public:
    virtual ~SmtTerm() {};
};


// class SmtTerm{
//     PyObject* data;

// public:
//     SmtTerm(PyObject* data) : data(data) { 
//         Py_INCREF(this->data); 
//     };
//     PyObject* getData() { 
//         Py_INCREF(this->data);
//         return data; 
//     };
//     ~SmtTerm(){ 
//         Py_DECREF(this->data); 
//     };
// };

class TermSubst{
public:
    virtual ~TermSubst(){};
};

// class TermSubst{
//     PyObject* subst;

// public:
//     TermSubst(PyObject* subst) : subst(subst) { Py_INCREF(this->subst); };
//     PyObject* getSubst() { 
//         Py_INCREF(this->subst);
//         return subst; 
//     };
// };

// class SmtModel{
// public:
//     PyObject* model;
//     SmtModel(PyObject* model) : model(model) {
//         Py_INCREF(this->model);
//     };
//     PyObject* getModel() {         
//         Py_INCREF(this->model);
//         return model; 
//     };
// };

// class SmtModel{
// public:
//     SmtModel() {};
//     void set(SmtTerm* k, SmtTerm* v) {         
//         // model[k] = v;
//         model.insert(std::pair<SmtTerm*, SmtTerm*>(k, v));
//         cout << "set done" << endl;
//     };
//     SmtTerm* get(SmtTerm* k){
//         auto it = model.find(k);
//         if (it != model.end()){
//             return it->second;
//         }
//         return nullptr;
//     }
//     std::vector<SmtTerm*>* keys(){
//         std::vector<SmtTerm*>* ks = new std::vector<SmtTerm*>();
//         for (auto &i : model){
//             ks->push_back(i.first);
//         }
//         return ks;
//     }

// private:
//     std::map<SmtTerm*, SmtTerm*> model;
// };

// class SmtModel{
// public:
//     SmtModel() {};
//     ~SmtModel() { 
//         for (auto &i : model){
//             delete i.first;
//             delete i.second;
//         }
//     };
//     void set(PyObject* k, PyObject* v) {         
//         // model[k] = v;
//         SmtTerm* tk = new SmtTerm(k);
//         SmtTerm* tv = new SmtTerm(v);
//         model.insert(std::pair<SmtTerm*, SmtTerm*>(tk, tv));
//         // cout << "set done" << endl;
//     };
//     SmtTerm* get(SmtTerm* k){
//         auto it = model.find(k);
//         if (it != model.end()){
//             return it->second;
//         }
//         return nullptr;
//     };
//     std::vector<SmtTerm*>* keys(){
//         std::vector<SmtTerm*>* ks = new std::vector<SmtTerm*>();
//         for (auto &i : model){
//             ks->push_back(i.first);
//         }
//         return ks;
//     };


// private:
//     std::map<SmtTerm*, SmtTerm*> model;
// };


class SmtModel{
public:
    virtual ~SmtModel() {};
    virtual SmtTerm* get(SmtTerm* k) = 0;
    virtual std::vector<SmtTerm*>* keys() = 0;
};



// template<typename T>
// class Model{
// public:
//     Model() {};
//     ~Model() { 
//         for (auto &i : model){
//             delete i.first;
//             delete i.second;
//         }
//     };
//     void set(T k, T v) {         
//         // model[k] = v;
//         model.insert(std::pair<T, T>(k, v));
//         // cout << "set done" << endl;
//     };
//     T get(T k){
//         auto it = model.find(k);
//         if (it != model.end()){
//             return it->second;
//         }
//         return nullptr;
//     };
//     std::vector<T>* keys(){
//         std::vector<T>* ks = new std::vector<T>();
//         for (auto &i : model){
//             ks->push_back(i.first);
//         }
//         return ks;
//     };

// private:
//     std::map<T, T> model;
// };

// typedef Model<SmtTerm*> SmtModel;


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
    virtual PyObject* prepareFor(VisibleModule* module) = 0;
    virtual SmtTerm* dag2term(DagNode* dag) = 0;
    virtual DagNode* term2dag(SmtTerm* term) = 0;
    virtual PyObject* mkApp(PyObject* symbol, PyObject* args) = 0;
    virtual PyObject* getSymbol(PyObject* dag) = 0;
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
    virtual void print_model() = 0;
    virtual void set_logic(const char* logic) = 0;
};

class WrapperFactory
{
public:
    virtual ~WrapperFactory() {};
    virtual Converter* createConverter() = 0;
    virtual Connector* createConnector() = 0;
};

#endif