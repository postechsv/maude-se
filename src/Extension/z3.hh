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
#include <memory>

#include "simpleRootContainer.hh"

class Z3SmtTerm : public SmtTerm
{
    std::shared_ptr<z3::context> ctx_;
    z3::expr expr_;

public:
    Z3SmtTerm(std::shared_ptr<z3::context> c, const z3::expr &e)
        : ctx_(std::move(c)), expr_(e) {}

    z3::expr expr() const { return expr_; }
    std::shared_ptr<z3::context> context() const { return ctx_; }
};

class Z3TermSubst : public TermSubst
{
public:
    explicit Z3TermSubst(std::shared_ptr<
                         std::map<std::shared_ptr<Z3SmtTerm>, std::shared_ptr<Z3SmtTerm>>>
                             subst_dict)
        : subst(std::move(subst_dict)) {}

    std::shared_ptr<std::map<std::shared_ptr<Z3SmtTerm>, std::shared_ptr<Z3SmtTerm>>> subst;
};


class Z3SmtModel : public SmtModel
{
public:
    Z3SmtModel(const z3::model &m, std::shared_ptr<z3::context> ctx)
        : model_(m), ctx_(std::move(ctx))
    {
        int num = m.num_consts();
        for (int i = 0; i < num; ++i)
        {
            z3::func_decl c = m.get_const_decl(i);
            z3::expr r = m.get_const_interp(c);

            auto lhs = std::make_shared<Z3SmtTerm>(ctx_, c());
            auto rhs = std::make_shared<Z3SmtTerm>(ctx_, r);

            model_map_[lhs] = rhs;
        }
    }

    SmtTerm *get(SmtTerm *k) override
    {
        auto *z3k = dynamic_cast<Z3SmtTerm *>(k);
        if (!z3k)
            return nullptr;

        unsigned id = z3k->expr().id();

        for (const auto &[lhs, rhs] : model_map_)
        {
            if (id == lhs->expr().id())
            {
                return rhs.get();
            }
        }
        return nullptr;
    }

    std::vector<SmtTerm *> *keys() override
    {
        auto *ks = new std::vector<SmtTerm *>();
        for (const auto &[lhs, _] : model_map_)
        {
            ks->push_back(lhs.get());
        }
        return ks;
    }

private:
    z3::model model_;
    std::shared_ptr<z3::context> ctx_;
    std::map<std::shared_ptr<Z3SmtTerm>, std::shared_ptr<Z3SmtTerm>> model_map_;
};

struct cmpExprById
{
    bool operator()(const z3::expr &lhs, const z3::expr &rhs) const
    {
        return lhs.id() < rhs.id();
    }
};

// Converter should be SimpleRootContainer because it contains variable DagNode maps.
// Otherwise, metaLevel operators such as metaSmtSearch would fail due to corrupted dags.
class Z3Converter : public Converter, public NativeSmtConverter<z3::expr, cmpExprById>, private SimpleRootContainer
{
public:
    Z3Converter(const SMT_Info &smtInfo);
    ~Z3Converter() {};
    void prepareFor(VisibleModule *module);
    SmtTerm *dag2term(DagNode *dag);
    DagNode *term2dag(SmtTerm *term);

public:
    inline std::shared_ptr<z3::context> getContext() { return ctx; };

private:
    std::shared_ptr<z3::context> ctx;
    SymbolGetter sg;

private:
    // override
    z3::expr makeVariable(DagNode *dag);

    // Aux
    z3::expr dag2termInternal(DagNode *dag);
    DagNode *term2dagInternal(z3::expr);

private:
    void markReachableNodes();
};

class Z3Connector : public Connector
{
public:
    Z3Connector(Z3Converter *conv);
    bool check_sat(std::vector<SmtTerm *> consts);
    bool subsume(TermSubst *subst, SmtTerm *prev, SmtTerm *acc, SmtTerm *cur);
    TermSubst *mk_subst(std::map<DagNode *, DagNode *> &subst_dict);
    SmtTerm *add_const(SmtTerm *acc, SmtTerm *cur);
    SmtModel *get_model();
    void push();
    void pop();

    void print_model() {};
    void set_logic(const char *logic);
    void reset();

    Converter *get_converter() { return conv; };

private:
    z3::expr translate(z3::expr e);

    std::shared_ptr<z3::context> ctx; // context shared
    std::unique_ptr<z3::solver> s;    // solver from external context
    Z3Converter *conv;
    int pushCount;
};

class Z3SmtManagerFactory : public SmtManagerFactory
{
public:
    Converter *createConverter(const SMT_Info &smtInfo)
    {
        return new Z3Converter(smtInfo);
    }
    Connector *createConnector(Converter *conv)
    {
        return new Z3Connector(dynamic_cast<Z3Converter *>(conv));
    }
};

class SmtManagerFactorySetter : public SmtManagerFactorySetterInterface
{
public:
    void set()
    {
        if (smtManagerFactory)
            delete smtManagerFactory;
        smtManagerFactory = new Z3SmtManagerFactory();
    };
};

#endif
