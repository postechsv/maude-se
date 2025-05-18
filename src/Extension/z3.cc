// utility stuff
#include "macros.hh"
#include "vector.hh"
#include "pointerMap.hh"
#include "meta.hh"

// forward declarations
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "higher.hh"
#include "freeTheory.hh"
#include "AU_Theory.hh"
#include "NA_Theory.hh"
#include "builtIn.hh"
#include "strategyLanguage.hh"
#include "mixfix.hh"
#include "SMT.hh"

// interface class definitions
#include "symbol.hh"
#include "term.hh"

// core class definitions
#include "rewritingContext.hh"
#include "symbolMap.hh"

#include "dagArgumentIterator.hh"
#include "argumentIterator.hh"

// variable class definitions
#include "variableSymbol.hh"
#include "variableDagNode.hh"
#include "variableTerm.hh"

#include "freeDagNode.hh"

// SMT class definitions
#include "SMT_Symbol.hh"
#include "SMT_NumberSymbol.hh"
#include "SMT_NumberDagNode.hh"

// front end class definitions
#include "token.hh"
#include "z3.hh"

#include "strategicTask.hh"
#include "strategicExecution.hh"
#include "strategyExpression.hh"

// //      interface class definitions
// #include "symbol.hh"
// #include "dagNode.hh"
// #include "rawDagArgumentIterator.hh"
// #include "rawArgumentIterator.hh"
// #include "term.hh"
// #include "extensionInfo.hh"

// //      core class definitions
// #include "variableInfo.hh"
// #include "variableSymbol.hh"
// #include "preEquation.hh"
// #include "substitution.hh"
// #include "rewritingContext.hh"
// #include "module.hh"
// #include "label.hh"
// #include "rule.hh"
// #include "symbolMap.hh"

// //	higher class definitions
// #include "pattern.hh"
// #include "rewriteSearchState.hh"
// #include "matchSearchState.hh"
// #include "rewriteSequenceSearch.hh"
// #include "narrowingSequenceSearch.hh"
// #include "unificationProblem.hh"
// #include "irredundantUnificationProblem.hh"
// #include "variantSearch.hh"
// #include "filteredVariantUnifierSearch.hh"
// #include "narrowingSearchState2.hh"
// #include "narrowingSequenceSearch3.hh"

// //      free theory class definitions
// #include "freeNet.hh"
// #include "freeSymbol.hh"
// #include "freeDagNode.hh"

// //      variable class definitions
// #include "variableDagNode.hh"

// //      built in class definitions
// #include "succSymbol.hh"
// #include "bindingMacros.hh"

//      front end class definitions
#include "userLevelRewritingContext.hh"
#include "quotedIdentifierSymbol.hh"
#include "quotedIdentifierDagNode.hh"
#include "quotedIdentifierOpSymbol.hh"
#include "metaModule.hh"
#include "metaLevel.hh"
#include "metaLevelSmtOpSymbol.hh"
#include "fileTable.hh"
#include "syntacticPreModule.hh"
#include "interpreter.hh"
#include "visibleModule.hh"
#include "freshVariableSource.hh"
#include "mixfixParser.hh"

#include "metaLevelSmtOpSymbol.hh"

// #include <fstream>

#include <chrono>

Z3Connector::Z3Connector(Z3Converter *conv)
    : pushCount(0),
      conv(conv), ctx(std::make_shared<z3::context>()), 
      s(new z3::solver(*conv->getContext())),
      s_v(std::make_unique<z3::solver>(*ctx)) {}

bool Z3Connector::check_sat(std::vector<SmtTerm *> consts)
{
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<Z3SmtTerm*> zterms;
    zterms.reserve(consts.size());

    for (auto* t : consts)
        zterms.push_back(static_cast<Z3SmtTerm*>(t)); 

    z3::expr_vector constraints(s->ctx());
    for (auto* zt : zterms)
        constraints.push_back(zt->expr());

    s->add(constraints); 

    auto mid = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    std::cout << "Elapsed time (add): " << duration1.count() << " ms\n";

    z3::check_result r = s->check();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);
    std::cout << "Elapsed time (check sat): " << duration.count() << " ms\n";

    switch (r)
    {
    case z3::unsat:
        return false;
    case z3::sat:
        return true;
    case z3::unknown:
        IssueWarning("Z3 reported an error - giving up:");
        return false;
    }
    IssueWarning("Z3 not able to determine satisfiability  - giving up.");
    return false;
}

SmtTerm *Z3Connector::add_const(SmtTerm *acc, SmtTerm *cur)
{
    if (acc == nullptr)
    {
        return cur;
    }
    else
    {
        Z3SmtTerm *z3Acc = dynamic_cast<Z3SmtTerm *>(acc);
        Z3SmtTerm *z3Cur = dynamic_cast<Z3SmtTerm *>(cur);
        return new Z3SmtTerm(conv->getContext(), z3Acc->expr() && z3Cur->expr());
    }
}

inline z3::expr Z3Connector::translate(z3::expr e)
{
    return z3::expr(*ctx, Z3_translate(*conv->getContext(), e, *ctx));
}

TermSubst* Z3Connector::mk_subst(std::map<DagNode*, DagNode*>& subst_dict)
{
    auto subst_map = std::make_shared<
        std::map<std::shared_ptr<Z3SmtTerm>, std::shared_ptr<Z3SmtTerm>>>();

    for (auto& [dag_lhs, dag_rhs] : subst_dict)
    {
        Z3SmtTerm* lhs_raw = dynamic_cast<Z3SmtTerm*>(conv->dag2term(dag_lhs));
        Z3SmtTerm* rhs_raw = dynamic_cast<Z3SmtTerm*>(conv->dag2term(dag_rhs));

        if (lhs_raw && rhs_raw) {
            auto lhs = std::make_shared<Z3SmtTerm>(*lhs_raw);  // use copy constructor
            auto rhs = std::make_shared<Z3SmtTerm>(*rhs_raw);
            (*subst_map)[lhs] = rhs;
        }
    }

    return new Z3TermSubst(subst_map);
}


bool Z3Connector::subsume(TermSubst *subst, SmtTerm *prev, SmtTerm *acc, SmtTerm *cur)
{
    Z3TermSubst *z3subst = dynamic_cast<Z3TermSubst *>(subst);

    z3::expr_vector from(*ctx);
    z3::expr_vector to(*ctx);

    for (auto it : *z3subst->subst)
    {
        from.push_back(translate(it.first->expr()));
        to.push_back(translate(it.second->expr()));
    }

    z3::expr z3_acc = translate(dynamic_cast<Z3SmtTerm *>(acc)->expr());
    z3::expr z3_cur = translate(dynamic_cast<Z3SmtTerm *>(cur)->expr());
    z3::expr z3_prv = translate(dynamic_cast<Z3SmtTerm *>(prev)->expr());

    z3::expr f = !(implies(z3_acc && z3_cur, z3_prv.substitute(from, to)));

    // Verbose("    Formula:");
    // Verbose("      acc: " << z3_acc);
    // Verbose("      cur: " << z3_cur);
    // Verbose("      prv: " << z3_prv);
    // Verbose("    Implication:");
    // Verbose("      " << f);

    s_v->push();
    s_v->add(f);
    z3::check_result r;
    try
    {
        r = s_v->check();
    }
    catch (z3::exception &ex)
    {
        IssueWarning("Z3 subsumption failed - " << ex.msg());
        // std::ofstream ofs;
        // ofs.open("debug.smt2");
        // ofs << s_v->to_smt2() << std::endl;
        // ofs.close();
        s_v->reset();
        return false;
    }
    s_v->pop();

    switch (r)
    {
    case z3::unsat:
        return true;
    case z3::sat:
        return false;
    default:
        IssueWarning("Subsumption result is unknown");
        return false;
    }
}

SmtModel *Z3Connector::get_model()
{
    return new Z3SmtModel(s->get_model(), conv->getContext());
}

void Z3Connector::push()
{
    s->push();
    ++pushCount;
}

void Z3Connector::pop()
{
    Assert(pushCount > 0, "bad pop");
    s->pop();
    --pushCount;
}

void Z3Connector::reset()
{
    pushCount = 0;
    s->reset();
}

void Z3Connector::set_logic(const char *logic)
{
    try
    {
        s = std::make_unique<z3::solver>(*conv->getContext(), logic);
        s_v = std::make_unique<z3::solver>(*ctx, logic);
    }
    catch (z3::exception &ex)
    {
        IssueWarning("Z3 solver failed - " << ex.msg());
    }
}

Z3Converter::Z3Converter(const SMT_Info &smtInfo)
    : ctx(std::make_shared<z3::context>()), NativeSmtConverter(smtInfo) {}

void Z3Converter::prepareFor(VisibleModule *module)
{
    sg.setModule(module);
}

SmtTerm *Z3Converter::dag2term(DagNode *dag)
{
    auto start = std::chrono::high_resolution_clock::now();
    z3::expr e = dag2termInternal(dag);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Elapsed time: " << duration.count() << " ms\n";
    return new Z3SmtTerm(ctx, e);
}

DagNode *Z3Converter::term2dag(SmtTerm *term)
{
    Z3SmtTerm *t = dynamic_cast<Z3SmtTerm *>(term);
    DagNode *d = term2dagInternal(t->expr());
    if (d->getSort() == nullptr)
    {
        RewritingContext *context = new UserLevelRewritingContext(d);
        d->computeTrueSort(*context);
        delete context;
    }
    return d;
}

void Z3Converter::markReachableNodes()
{
    for (auto it = smtManagerVariableMap.begin(); it != smtManagerVariableMap.end(); it++)
    {
        it->first->mark();
    }
}

z3::expr Z3Converter::dag2termInternal(DagNode *dag)
{
    if (SMT_NumberDagNode *n = dynamic_cast<SMT_NumberDagNode *>(dag))
    {
        mpq_class value = n->getValue();
        Sort *sort = n->symbol()->getRangeSort();
        if (smtInfo.getType(sort) == SMT_Info::INTEGER)
        {
            return ctx->int_val(value.get_str().c_str());
        }
        else if (smtInfo.getType(sort) == SMT_Info::REAL)
        {
            return ctx->real_val(value.get_str().c_str());
        }
    }

    // if (VariableDagNode *v = dynamic_cast<VariableDagNode *>(dag))
    // {
    //     return makeVariable(v);
    // }

    try
    {
        return makeVariable(dag);
    }
    catch (...)
    {
        // ignore
    }

    int nrArgs = dag->symbol()->arity();

    // need to be initialized with original ctx.
    std::vector<z3::expr> exprs;

    FreeDagNode *f = safeCast(FreeDagNode *, dag);
    for (int i = 0; i < nrArgs; ++i)
    {
        exprs.push_back(dag2termInternal(f->getArgument(i)));
    }

    if (SMT_Symbol *s = dynamic_cast<SMT_Symbol *>(dag->symbol()))
    {
        switch (s->getOperator())
        {
        //
        //	Boolean stuff.
        //
        case SMT_Symbol::CONST_TRUE:
        {
            return ctx->bool_val(true);
        }
        case SMT_Symbol::CONST_FALSE:
        {
            return ctx->bool_val(false);
        }
        case SMT_Symbol::NOT:
        {
            return !exprs[0];
        }

        case SMT_Symbol::AND:
        {
            return exprs[0] && exprs[1];
        }
        case SMT_Symbol::OR:
        {
            return exprs[0] || exprs[1];
        }
        case SMT_Symbol::XOR:
        {
            return exprs[0] ^ exprs[1];
        }
        case SMT_Symbol::IMPLIES:
        {
            return implies(exprs[0], exprs[1]);
        }
            //
            //	Polymorphic Boolean stuff.
            //
        case SMT_Symbol::EQUALS:
        {
            return exprs[0] == exprs[1];
        }
        case SMT_Symbol::NOT_EQUALS:
        {
            return exprs[0] != exprs[1];
        }
        case SMT_Symbol::ITE:
        {
            return ite(exprs[0], exprs[1], exprs[2]);
        }
            //
            //	Integer stuff.
            //
        case SMT_Symbol::UNARY_MINUS:
        {
            return -exprs[0];
        }
        case SMT_Symbol::MINUS:
        {
            return exprs[0] - exprs[1];
        }
        case SMT_Symbol::PLUS:
        {
            return exprs[0] + exprs[1];
        }
        case SMT_Symbol::MULT:
        {
            return exprs[0] * exprs[1];
        }
        case SMT_Symbol::DIV:
        {
            return exprs[0] / exprs[1];
        }
        case SMT_Symbol::MOD:
        {
            return mod(exprs[0], exprs[1]);
        }
            //
            //	Integer tests.
            //

        case SMT_Symbol::LT:
        {
            return exprs[0] < exprs[1];
        }
        case SMT_Symbol::LEQ:
        {
            return exprs[0] <= exprs[1];
        }
        case SMT_Symbol::GT:
        {
            return exprs[0] > exprs[1];
        }
        case SMT_Symbol::GEQ:
        {
            return exprs[0] >= exprs[1];
        }

        case SMT_Symbol::DIVISIBLE:
        {
            //
            //	Second argument must be a positive constant.
            //	Typing ensures it is an integer.
            //
            DagNode *a = f->getArgument(1);
            if (SMT_NumberDagNode *n = dynamic_cast<SMT_NumberDagNode *>(a))
            {
                const mpq_class &rat = n->getValue();
                if (rat > 0)
                {
                    return exprs[1] / exprs[0];
                }
            }
            IssueWarning("bad divisor in " << QUOTE(dag) << ".");
            goto fail;
        }
            //
            //	Stuff that is extra to reals.
            //
        case SMT_Symbol::REAL_DIVISION:
        {
            return exprs[0] / exprs[1];
        }
        case SMT_Symbol::TO_REAL:
        {
            return z3::to_real(exprs[0]);
        }
        case SMT_Symbol::TO_INTEGER:
        {
            Z3_ast result = Z3_mk_real2int(exprs[0].ctx(), exprs[0]);
            return z3::expr(exprs[0].ctx(), result);
        }
        case SMT_Symbol::IS_INTEGER:
        {
            return z3::is_int(exprs[0]);
        }
        }
    }
    else
    {
        // if(dag->symbol() == extensionSymbol->forallBoolSymbol ||
        //     dag->symbol() == extensionSymbol->forallIntSymbol ||
        //     dag->symbol() == extensionSymbol->forallRealSymbol){
        //     return new Z3SmtTerm(forall(exprs[0], exprs[1]));
        // }

        // if(dag->symbol() == extensionSymbol->existsBoolSymbol ||
        //     dag->symbol() == extensionSymbol->existsIntSymbol ||
        //     dag->symbol() == extensionSymbol->existsRealSymbol){
        //     return new Z3SmtTerm(exists(exprs[0], exprs[1]));
        // }
    }
    IssueWarning("term " << QUOTE(dag) << " is not a valid SMT term.");
fail:
    throw std::runtime_error("not a valid term, return original term instead");
}

z3::expr Z3Converter::makeVariable(DagNode *dag)
{
    // Two dag nodes are the same
    auto it = smtManagerVariableMap.find(dag);
    if (it != smtManagerVariableMap.end())
        return it->second;

    // Two dag nodes are different while they both point to the same symbol
    for (auto it = smtManagerVariableMap.begin(); it != smtManagerVariableMap.end(); it++)
    {
        if (dag->equal(it->first))
        {
            smtManagerVariableMap.insert(SmtManagerVariableMap::value_type(dag, it->second));
            return it->second;
        }
    }
    z3::sort type(*ctx);
    string name;

    if (VariableDagNode *v = dynamic_cast<VariableDagNode *>(dag))
    {
        Symbol *s = v->symbol();

        Sort *sort = s->getRangeSort();
        int id = v->id();
        name = Token::name(id);

        switch (smtInfo.getType(sort))
        {
        case SMT_Info::NOT_SMT:
        {
            IssueWarning("Variable " << QUOTE(static_cast<DagNode *>(v)) << " does not belong to an SMT sort.");
            throw std::runtime_error("failed to generate SMT variable");
        }
        case SMT_Info::BOOLEAN:
        {
            type = ctx->bool_sort();
            name = name + "_" + string("Boolean");
            break;
        }
        case SMT_Info::INTEGER:
        {
            type = ctx->int_sort();
            name = name + "_" + string("Integer");
            break;
        }
        case SMT_Info::REAL:
        {
            type = ctx->real_sort();
            name = name + "_" + string("Real");
            break;
        }
        }
    }
    else
    {
        Sort *smtVarId = sg.getSort("SMTVarId");
        Sort *bvS = sg.getSort("BooleanVar");
        Sort *ivS = sg.getSort("IntegerVar");
        Sort *rvS = sg.getSort("RealVar");

        if (smtVarId == nullptr)
            throw std::runtime_error("not an SMT variable");

        Symbol *symbol = dag->symbol();
        Vector<ConnectedComponent *> dom;
        dom.push_back(smtVarId->component());

        bool varSet = false;

        if (bvS && !varSet)
        {
            if (symbol == sg.getSymbol("b", dom, bvS->component()))
            {
                type = ctx->bool_sort();
                name = "b_";

                varSet = true;
            }
        }

        if (ivS && !varSet)
        {
            if (symbol == sg.getSymbol("i", dom, ivS->component()))
            {
                type = ctx->int_sort();
                name = "i_";

                varSet = true;
            }
        }

        if (rvS && !varSet)
        {
            if (symbol == sg.getSymbol("r", dom, rvS->component()))
            {
                type = ctx->real_sort();
                name = "r_";

                varSet = true;
            }
        }

        if (!varSet)
        {
            throw std::runtime_error("failed to generate SMT variable");
        }

        const void *address = static_cast<const void *>(dag);
        std::stringstream ss;
        ss << address;
        string varId = ss.str();
        name += varId;
    }

    z3::expr newVar = ctx->constant(name.c_str(), type);
    smtManagerVariableMap.insert(SmtManagerVariableMap::value_type(dag, newVar));
    return newVar;
}

DagNode *Z3Converter::term2dagInternal(z3::expr e)
{
    ReverseSmtManagerVariableMap *rsv = generateReverseVariableMap();
    if (rsv != nullptr)
    {
        auto it = rsv->find(e);
        if (it != rsv->end())
        {
            return it->second;
        }
    }

    // z3::expr e = dynamic_cast<Z3SmtTerm*>(term)->getTerm();

    if (e.is_true())
    {
        Vector<ConnectedComponent *> dom;
        ConnectedComponent *r = sg.getKind("Boolean");
        return sg.getSymbol("true", dom, r)->makeDagNode();
    }

    if (e.is_false())
    {
        Vector<ConnectedComponent *> dom;
        ConnectedComponent *r = sg.getKind("Boolean");
        return sg.getSymbol("false", dom, r)->makeDagNode();
    }

    // if (e.is_forall())
    // {
    //     unsigned num = Z3_get_quantifier_num_bound(ctx, e);
    //     DagNode *prev = nullptr;
    //     for (unsigned i = 0; i < num; i++)
    //     {
    //         DagNode *q_var = nullptr;
    //         Symbol *symbol = nullptr;

    //         unsigned at = (num - i) - 1;
    //         Z3_sort_kind type = Z3_get_sort_kind(ctx, Z3_get_quantifier_bound_sort(ctx, e, at));

    //         // make id, use "i" instead of "at"
    //         // because z3 use right-to-left de-Bruijn index
    //         std::stringstream ss;
    //         ss << i;
    //         string varId = ss.str();

    //         mpq_t total;
    //         mpq_init(total);
    //         mpq_set_str(total, varId.c_str(), 10);
    //         mpq_canonicalize(total);
    //         mpq_class mpNum(total);

    //         Vector<DagNode *> id_args(1);
    //         Vector<ConnectedComponent*> dom;
    //         ConnectedComponent* r = getSort("Boolean")->component();

    //         id_args[0] = new SMT_NumberDagNode(extensionSymbol->integerSymbol, mpNum);
    //         // if (type == Z3_BOOL_SORT){
    //         //     q_var = extensionSymbol->freshBoolVarSymbol->makeDagNode(id_args);
    //         //     symbol = extensionSymbol->forallBoolSymbol;
    //         // } else if (type == Z3_INT_SORT){
    //         //     q_var = extensionSymbol->freshIntVarSymbol->makeDagNode(id_args);
    //         //     symbol = extensionSymbol->forallIntSymbol;
    //         // } else if (type == Z3_REAL_SORT){
    //         //     q_var = extensionSymbol->freshRealVarSymbol->makeDagNode(id_args);
    //         //     symbol = extensionSymbol->forallRealSymbol;
    //         // } else {
    //         //     // raise error
    //         // }

    //         if (q_var == nullptr || symbol == nullptr)
    //         {
    //             // raise error
    //         }

    //         Vector<DagNode *> arg(2);
    //         arg[0] = q_var;

    //         if (prev == nullptr)
    //         {
    //             arg[1] = term2dagInternal(e.body());
    //             prev = symbol->makeDagNode(arg);
    //         }
    //         else
    //         {
    //             arg[1] = prev;
    //             prev = symbol->makeDagNode(arg);
    //         }
    //     }

    //     if (prev == nullptr)
    //     {
    //         // raise error
    //     }

    //     return prev;
    // }

    // if (e.is_exists())
    // {
    //     unsigned num = Z3_get_quantifier_num_bound(ctx, e);
    //     DagNode *prev = nullptr;
    //     for (unsigned i = 0; i < num; i++)
    //     {
    //         DagNode *q_var = nullptr;
    //         Symbol *symbol = nullptr;

    //         unsigned at = (num - i) - 1;
    //         Z3_sort_kind type = Z3_get_sort_kind(ctx, Z3_get_quantifier_bound_sort(ctx, e, at));

    //         // make id, use "i" instead of "at"
    //         // because z3 use right-to-left de-Bruijn index
    //         std::stringstream ss;
    //         ss << i;
    //         string varId = ss.str();

    //         mpq_t total;
    //         mpq_init(total);
    //         mpq_set_str(total, varId.c_str(), 10);
    //         mpq_canonicalize(total);
    //         mpq_class mpNum(total);

    //         Vector<DagNode *> id_args(1);
    //         id_args[0] = new SMT_NumberDagNode(extensionSymbol->integerSymbol, mpNum);
    //         // if (type == Z3_BOOL_SORT){
    //         //     q_var = extensionSymbol->freshBoolVarSymbol->makeDagNode(id_args);
    //         //     symbol = extensionSymbol->existsBoolSymbol;
    //         // } else if (type == Z3_INT_SORT){
    //         //     q_var = extensionSymbol->freshIntVarSymbol->makeDagNode(id_args);
    //         //     symbol = extensionSymbol->existsIntSymbol;
    //         // } else if (type == Z3_REAL_SORT){
    //         //     q_var = extensionSymbol->freshRealVarSymbol->makeDagNode(id_args);
    //         //     symbol = extensionSymbol->existsRealSymbol;
    //         // } else {
    //         //     // raise error
    //         // }

    //         if (q_var == nullptr || symbol == nullptr)
    //         {
    //             // raise error
    //         }

    //         Vector<DagNode *> arg(2);
    //         arg[0] = q_var;

    //         if (prev == nullptr)
    //         {
    //             arg[1] = term2dagInternal(e.body());
    //             prev = symbol->makeDagNode(arg);
    //         }
    //         else
    //         {
    //             arg[1] = prev;
    //             prev = symbol->makeDagNode(arg);
    //         }
    //     }

    //     if (prev == nullptr)
    //     {
    //         // raise error
    //     }

    //     return prev;
    // }

    if (e.is_not())
    {
        Vector<DagNode *> arg(1);
        arg[0] = term2dagInternal(e.arg(0));

        ConnectedComponent *bk = sg.getKind("BooleanExpr");
        Vector<ConnectedComponent *> domain;
        domain.push_back(bk);

        return sg.getSymbol("not_", domain, bk)->makeDagNode(arg);
    }

    if (e.is_and() || e.is_or() || e.is_xor() || e.is_implies())
    {
        unsigned child_num = e.num_args();
        Assert(child_num == 2, "wrong children");

        Vector<DagNode *> arg(child_num);
        for (unsigned i = 0; i < child_num; i++)
        {
            arg[i] = term2dagInternal(e.arg(i));
        }

        ConnectedComponent *bk = sg.getKind("BooleanExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(bk);
        domain.push_back(bk);

        return sg.getSymbol("_and_", domain, bk)->makeDagNode(arg);
    }

    if (e.is_eq())
    {
        z3::expr e1 = e.arg(0);
        z3::expr e2 = e.arg(1);

        bool check_bool_eq = (!e1.is_bool() || e2.is_bool()) && (e1.is_bool() || !e2.is_bool());
        bool check_int_eq = (!e1.is_int() || e2.is_int()) && (e1.is_int() || !e2.is_int());
        bool check_real_eq = (!e1.is_real() || e2.is_real()) && (e1.is_real() || !e2.is_real());

        Assert(check_bool_eq && check_int_eq && check_real_eq, "lhs and rhs should have the same type");

        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e1);
        arg[1] = term2dagInternal(e2);

        ConnectedComponent *bk = sg.getKind("BooleanExpr");

        if (e1.is_bool())
        {
            Vector<ConnectedComponent *> domain;

            domain.push_back(bk);
            domain.push_back(bk);

            return sg.getSymbol("_===_", domain, bk)->makeDagNode(arg);
        }

        if (e1.is_int())
        {
            ConnectedComponent *ik = sg.getKind("IntegerExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(ik);
            domain.push_back(ik);

            return sg.getSymbol("_===_", domain, bk)->makeDagNode(arg);
        }

        if (e1.is_real())
        {
            ConnectedComponent *rk = sg.getKind("RealExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(rk);
            domain.push_back(rk);

            return sg.getSymbol("_===_", domain, bk)->makeDagNode(arg);
        }
    }

    // boolean
    if (e.is_ite())
    {
        Vector<DagNode *> arg(3);

        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));
        arg[2] = term2dagInternal(e.arg(2));

        ConnectedComponent *bk = sg.getKind("BooleanExpr");
        Vector<ConnectedComponent *> domain;
        if (e.arg(1).is_int())
        {
            ConnectedComponent *ik = sg.getKind("IntegerExpr");

            domain.push_back(ik);
            domain.push_back(ik);

            return sg.getSymbol("_?_:_", domain, bk)->makeDagNode(arg);
        }

        if (e.arg(1).is_real())
        {
            ConnectedComponent *ik = sg.getKind("RealExpr");

            domain.push_back(ik);
            domain.push_back(ik);

            return sg.getSymbol("_?_:_", domain, bk)->makeDagNode(arg);
        }

        if (e.arg(1).is_bool())
        {
            domain.push_back(bk);
            domain.push_back(bk);

            return sg.getSymbol("_?_:_", domain, bk)->makeDagNode(arg);
        }
    }

    // boolean type
    if (e.is_app() && Z3_OP_LT == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));

        ConnectedComponent *bk = sg.getKind("BooleanExpr");

        if (e.arg(0).is_int())
        {
            ConnectedComponent *ik = sg.getKind("IntegerExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(ik);
            domain.push_back(ik);

            return sg.getSymbol("_<_", domain, bk)->makeDagNode(arg);
        }
        else
        {
            ConnectedComponent *rk = sg.getKind("RealExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(rk);
            domain.push_back(rk);

            return sg.getSymbol("_<_", domain, bk)->makeDagNode(arg);
        }
    }

    if (e.is_app() && Z3_OP_LE == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));

        ConnectedComponent *bk = sg.getKind("BooleanExpr");

        if (e.arg(0).is_int())
        {
            ConnectedComponent *ik = sg.getKind("IntegerExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(ik);
            domain.push_back(ik);

            return sg.getSymbol("_<=_", domain, bk)->makeDagNode(arg);
        }
        else
        {
            ConnectedComponent *rk = sg.getKind("RealExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(rk);
            domain.push_back(rk);

            return sg.getSymbol("_<=_", domain, bk)->makeDagNode(arg);
        }
    }

    if (e.is_app() && Z3_OP_GT == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));

        ConnectedComponent *bk = sg.getKind("BooleanExpr");

        if (e.arg(0).is_int())
        {
            ConnectedComponent *ik = sg.getKind("IntegerExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(ik);
            domain.push_back(ik);

            return sg.getSymbol("_>_", domain, bk)->makeDagNode(arg);
        }
        else
        {
            ConnectedComponent *rk = sg.getKind("RealExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(rk);
            domain.push_back(rk);

            return sg.getSymbol("_>_", domain, bk)->makeDagNode(arg);
        }
    }

    if (e.is_app() && Z3_OP_GE == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));

        ConnectedComponent *bk = sg.getKind("BooleanExpr");

        if (e.arg(0).is_int())
        {
            ConnectedComponent *ik = sg.getKind("IntegerExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(ik);
            domain.push_back(ik);

            return sg.getSymbol("_>=_", domain, bk)->makeDagNode(arg);
        }
        else
        {
            ConnectedComponent *rk = sg.getKind("RealExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(rk);
            domain.push_back(rk);

            return sg.getSymbol("_>=_", domain, bk)->makeDagNode(arg);
        }
    }

    if (e.is_app() && Z3_OP_EQ == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));

        ConnectedComponent *bk = sg.getKind("BooleanExpr");

        if (e.arg(0).is_int())
        {
            ConnectedComponent *ik = sg.getKind("IntegerExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(ik);
            domain.push_back(ik);

            return sg.getSymbol("_===_", domain, bk)->makeDagNode(arg);
        }
        else
        {
            ConnectedComponent *rk = sg.getKind("RealExpr");
            Vector<ConnectedComponent *> domain;

            domain.push_back(rk);
            domain.push_back(rk);

            return sg.getSymbol("_===_", domain, bk)->makeDagNode(arg);
        }
    }

    if (e.is_app() && Z3_OP_IS_INT == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(1);
        arg[0] = term2dagInternal(e.arg(0));

        ConnectedComponent *bk = sg.getKind("BooleanExpr");
        ConnectedComponent *rk = sg.getKind("RealExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(rk);

        return sg.getSymbol("isInteger", domain, bk)->makeDagNode(arg);
    }

    /**
     * Integer operation
     */
    if (e.is_app() && Z3_OP_TO_INT == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(1);
        arg[0] = term2dagInternal(e.arg(0));

        ConnectedComponent *rk = sg.getKind("RealExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(rk);

        return sg.getSymbol("toInteger", domain, rk)->makeDagNode(arg);
    }
    if (e.is_int() && e.is_app() && Z3_OP_UMINUS == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(1);
        arg[0] = term2dagInternal(e.arg(0));

        ConnectedComponent *ik = sg.getKind("IntegerExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(ik);

        return sg.getSymbol("-_", domain, ik)->makeDagNode(arg);
    }

    if (e.is_int() && e.is_app() && Z3_OP_ADD == e.decl().decl_kind())
    {
        unsigned child_num = e.num_args();
        Assert(child_num == 2, "wrong children");

        Vector<DagNode *> arg(child_num);
        for (unsigned i = 0; i < child_num; i++)
        {
            arg[i] = term2dagInternal(e.arg(i));
        }

        ConnectedComponent *ik = sg.getKind("IntegerExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(ik);
        domain.push_back(ik);

        return sg.getSymbol("_+_", domain, ik)->makeDagNode(arg);
    }

    if (e.is_int() && e.is_app() && Z3_OP_SUB == e.decl().decl_kind())
    {
        unsigned child_num = e.num_args();
        Assert(child_num == 2, "wrong children");

        Vector<DagNode *> arg(child_num);
        for (unsigned i = 0; i < child_num; i++)
        {
            arg[i] = term2dagInternal(e.arg(i));
        }

        ConnectedComponent *ik = sg.getKind("IntegerExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(ik);
        domain.push_back(ik);

        return sg.getSymbol("_-_", domain, ik)->makeDagNode(arg);
    }

    if (e.is_int() && e.is_app() && Z3_OP_MUL == e.decl().decl_kind())
    {
        unsigned child_num = e.num_args();
        Assert(child_num == 2, "wrong children");

        Vector<DagNode *> arg(child_num);
        for (unsigned i = 0; i < child_num; i++)
        {
            arg[i] = term2dagInternal(e.arg(i));
        }

        ConnectedComponent *ik = sg.getKind("IntegerExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(ik);
        domain.push_back(ik);

        return sg.getSymbol("_*_", domain, ik)->makeDagNode(arg);
    }

    if (e.is_int() && e.is_app() && Z3_OP_DIV == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));

        ConnectedComponent *ik = sg.getKind("IntegerExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(ik);
        domain.push_back(ik);

        return sg.getSymbol("_div_", domain, ik)->makeDagNode(arg);
    }

    if (e.is_int() && e.is_app() && Z3_OP_MOD == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));

        ConnectedComponent *ik = sg.getKind("IntegerExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(ik);
        domain.push_back(ik);

        return sg.getSymbol("_mod_", domain, ik)->makeDagNode(arg);
    }

    if (e.is_int() && e.is_numeral())
    {
        mpq_t total;
        mpq_init(total);
        mpq_set_str(total, e.get_decimal_string(0).c_str(), 10);
        mpq_canonicalize(total);
        mpq_class mpNum(total);

        ConnectedComponent *ik = sg.getKind("Integer");
        Vector<ConnectedComponent *> domain;

        Symbol *symbol = sg.getSymbol("<Integers>", domain, ik);
        return new SMT_NumberDagNode(static_cast<SMT_NumberSymbol *>(symbol), mpNum);
    }

    /**
     * Real type
     */
    if (e.is_app() && Z3_OP_TO_REAL == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(1);
        arg[0] = term2dagInternal(e.arg(0));

        ConnectedComponent *ik = sg.getKind("IntegerExpr");
        ConnectedComponent *rk = sg.getKind("RealExpr");

        Vector<ConnectedComponent *> domain;

        domain.push_back(ik);

        return sg.getSymbol("toReal", domain, rk)->makeDagNode(arg);
    }

    if (e.is_real() && e.is_app() && Z3_OP_UMINUS == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(1);
        arg[0] = term2dagInternal(e.arg(0));

        ConnectedComponent *rk = sg.getKind("RealExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(rk);

        return sg.getSymbol("-_", domain, rk)->makeDagNode(arg);
    }

    if (e.is_real() && e.is_app() && Z3_OP_ADD == e.decl().decl_kind())
    {
        unsigned child_num = e.num_args();
        Assert(child_num == 2, "wrong children");

        Vector<DagNode *> arg(child_num);
        for (unsigned i = 0; i < child_num; i++)
        {
            arg[i] = term2dagInternal(e.arg(i));
        }
        ConnectedComponent *rk = sg.getKind("RealExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(rk);
        domain.push_back(rk);

        return sg.getSymbol("_+_", domain, rk)->makeDagNode(arg);
    }

    if (e.is_real() && e.is_app() && Z3_OP_SUB == e.decl().decl_kind())
    {
        unsigned child_num = e.num_args();
        Assert(child_num == 2, "wrong children");

        Vector<DagNode *> arg(child_num);
        for (unsigned i = 0; i < child_num; i++)
        {
            arg[i] = term2dagInternal(e.arg(i));
        }
        ConnectedComponent *rk = sg.getKind("RealExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(rk);
        domain.push_back(rk);

        return sg.getSymbol("_-_", domain, rk)->makeDagNode(arg);
    }

    if (e.is_real() && e.is_app() && Z3_OP_MUL == e.decl().decl_kind())
    {
        unsigned child_num = e.num_args();
        Assert(child_num == 2, "wrong children");

        Vector<DagNode *> arg(child_num);
        for (unsigned i = 0; i < child_num; i++)
        {
            arg[i] = term2dagInternal(e.arg(i));
        }
        ConnectedComponent *rk = sg.getKind("RealExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(rk);
        domain.push_back(rk);

        return sg.getSymbol("_*_", domain, rk)->makeDagNode(arg);
    }

    if (e.is_real() && e.is_app() && Z3_OP_DIV == e.decl().decl_kind())
    {
        Vector<DagNode *> arg(2);
        arg[0] = term2dagInternal(e.arg(0));
        arg[1] = term2dagInternal(e.arg(1));

        ConnectedComponent *rk = sg.getKind("RealExpr");
        Vector<ConnectedComponent *> domain;

        domain.push_back(rk);
        domain.push_back(rk);

        return sg.getSymbol("_/_", domain, rk)->makeDagNode(arg);
    }

    if (e.is_real() && e.is_numeral())
    {
        mpq_t total;
        mpq_init(total);
        string newNum(e.numerator().get_decimal_string(0) + "/" + e.denominator().get_decimal_string(0));
        mpq_set_str(total, newNum.c_str(), 10);
        mpq_canonicalize(total);
        mpq_class mpNum(total);

        ConnectedComponent *rk = sg.getKind("Real");
        Vector<ConnectedComponent *> domain;

        Symbol *symbol = sg.getSymbol("<Reals>", domain, rk);
        return new SMT_NumberDagNode(static_cast<SMT_NumberSymbol *>(symbol), mpNum);
    }

    // for fresh quantified variable
    // if (e.is_var())
    // {
    //     unsigned index = Z3_get_index_value(ctx, e);

    //     std::stringstream ss;
    //     ss << index;
    //     string varId = ss.str();

    //     Vector<DagNode *> arg(1);

    //     mpq_t total;
    //     mpq_init(total);
    //     mpq_set_str(total, varId.c_str(), 10);
    //     mpq_canonicalize(total);
    //     mpq_class mpNum(total);
    //     arg[0] = new SMT_NumberDagNode(extensionSymbol->integerSymbol, mpNum);

    //     // if (e.is_int()){
    //     //     return extensionSymbol->freshIntVarSymbol->makeDagNode(arg);
    //     // } else if (e.is_real()){
    //     //     return extensionSymbol->freshRealVarSymbol->makeDagNode(arg);
    //     // } else if (e.is_bool()){
    //     //     return extensionSymbol->freshBoolVarSymbol->makeDagNode(arg);
    //     // } else {
    //     //     // raise exception
    //     // }
    // }
    throw std::runtime_error("fail to create Maude term");
}
