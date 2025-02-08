/*

    This file is part of the Maude 2 interpreter.

    Copyright 1997-2017 SRI International, Menlo Park, CA 94025, USA.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

*/

//
//      Class for generating SMT variables, version for CVC4 support.
//
#ifndef _z3_smt_hh_
#define _z3_smt_hh_
#include "z3++.h"
#include "smtInterface.hh"
#include "nativeSmt.hh"
#include "extGlobal.hh"
#include <vector>
#include <sstream>
#include <gmpxx.h>

class Z3SmtTerm : public SmtTerm, public z3::expr
{
public:
    Z3SmtTerm(z3::expr e) : z3::expr(e) {};
    ~Z3SmtTerm(){};

    inline z3::expr* to_z3_expr(){
        return dynamic_cast<z3::expr*>(this);
    };

// public:
//     Z3SmtTerm(z3::expr const & z3term) : z3term(z3term) {};
//     ~Z3SmtTerm(){};
//     inline z3::expr getTerm(){ return z3::expr(z3term.ctx()); };

// private:
//     z3::expr const & z3term;
// public:
//     Z3SmtTerm(z3::context & ctx) : ctx(ctx) {};
//     ~Z3SmtTerm(){};
//     inline z3::expr getTerm(){ return z3::expr(ctx); };

// private:
    // z3::context & ctx;

// public:
//     Z3SmtTerm(z3::expr const & z3term) : ctx(z3term.ctx()) {};
//     ~Z3SmtTerm(){};
//     inline z3::expr getTerm(){ return z3::expr(ctx); };

// private:
//     z3::context & ctx;

// public:
//     Z3SmtTerm(z3::expr const & z3term) { term = & z3::expr(z3term); };
//     ~Z3SmtTerm(){};
//     inline z3::expr getTerm(){ return z3::expr(*term); };

// private:
//     z3::expr* term;
};

// comparator for Reverse Variable Map
// struct rsvCmp {
//     bool operator()(const SmtTerm* lhs, const SmtTerm* rhs) const {
//         z3::expr a = dynamic_cast<Z3SmtTerm*>(const_cast<SmtTerm*>(lhs))->getTerm();
//         z3::expr b = dynamic_cast<Z3SmtTerm*>(const_cast<SmtTerm*>(rhs))->getTerm();
//         return a.id() < b.id();
//     }
// };

class Z3SmtModel : public SmtModel, public z3::model
{
public:
    Z3SmtModel(z3::model m) : z3::model(m) {
        // cout << "z3 model gen" << endl;
        int num = m.num_consts();

        for(int i = 0; i < num; i++){
            z3::func_decl c = m.get_const_decl(i);
            z3::expr r = m.get_const_interp(c);

            // cout << "  lhs: " << c() << endl;
            // cout << "  rhs: " << r << endl;

            Z3SmtTerm* lhs = new Z3SmtTerm(c());
            Z3SmtTerm* rhs = new Z3SmtTerm(r);

            model.insert(std::pair<Z3SmtTerm*, Z3SmtTerm*>(lhs, rhs));
        }

    };

    ~Z3SmtModel(){        
        // cout << "z3 model del" << endl;
        for (auto &i : model){
            delete i.first;
            delete i.second;
        }
    };

    SmtTerm* get(SmtTerm* k){
        if (Z3SmtTerm* t = static_cast<Z3SmtTerm*>(k)){
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
    std::map<Z3SmtTerm*, Z3SmtTerm*> model;
};

struct cmpExprById{
    bool operator( )(const z3::expr &lhs, const z3::expr &rhs) const {
        return lhs.id() < rhs.id();
    }
};


class Z3Converter : public Converter, public NativeSmtConverter< z3::expr, cmpExprById >
{
public:
    Z3Converter(const SMT_Info &smtInfo, MetaLevelSmtOpSymbol* extensionSymbol);
	~Z3Converter(){};
    void prepareFor(VisibleModule* module){};
    SmtTerm* dag2term(DagNode* dag){
        z3::expr e = dag2termInternal(dag);
        return new Z3SmtTerm(e);
    };
    DagNode* term2dag(SmtTerm* term){
        Z3SmtTerm* t = dynamic_cast<Z3SmtTerm*>(term);
        return term2dagInternal(z3::expr(*dynamic_cast<z3::expr*>(t)));
    }

public:
    inline z3::context* getContext(){ return &ctx; };

private:
    z3::context ctx;

private:
    // override
    z3::expr variableGenerator(DagNode *dag, ExprType exprType);
    z3::expr makeVariable(VariableDagNode* v);

    // Aux
    z3::expr dag2termInternal(DagNode* dag);
    DagNode* term2dagInternal(z3::expr);
};


class Z3Connector : public Connector
{
public:
    Z3Connector(Z3Converter* conv);
	~Z3Connector();
    bool check_sat(std::vector<SmtTerm*> consts);
    bool subsume(TermSubst* subst, SmtTerm* prev, SmtTerm* acc, SmtTerm* cur){ return false; };
    TermSubst* mk_subst(std::map<DagNode*, DagNode*>& subst_dict){ return nullptr; };
    SmtTerm* add_const(SmtTerm* acc, SmtTerm* cur);
    SmtModel* get_model();
    void push();
    void pop();

    void print_model(){};
    void set_logic(const char* logic);
    void reset();

    Converter* get_converter(){ return conv; };

private:
    z3::solver *s;
    int pushCount;
    Z3Converter* conv;
};

class Z3SmtManagerFactory : public SmtManagerFactory
{
public:
    Converter* createConverter(const SMT_Info& smtInfo, MetaLevelSmtOpSymbol* extensionSymbol){
        return new Z3Converter(smtInfo, extensionSymbol);
    }
    Connector* createConnector(Converter* conv){
        return new Z3Connector(dynamic_cast<Z3Converter*>(conv));
    }
};

class SmtManagerFactorySetter : public SmtManagerFactorySetterInterface
{
public:
    void set(){
        if (smtManagerFactory) delete smtManagerFactory;
        smtManagerFactory = new Z3SmtManagerFactory();
    };
};

#endif
