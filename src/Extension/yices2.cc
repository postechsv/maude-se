#include "macros.hh"
#include "vector.hh"
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "mixfix.hh"
#include "SMT.hh"
#include "symbol.hh"
#include "term.hh"
#include "variableDagNode.hh"
#include "freeDagNode.hh"
#include "SMT_Symbol.hh"
#include "SMT_NumberSymbol.hh"
#include "SMT_NumberDagNode.hh"
#include "token.hh"
#include "yices2.hh"
#include "userLevelRewritingContext.hh"
#include <mutex>
#include <stdexcept>

namespace
{
term_t checked(term_t value)
{
    if (value == NULL_TERM)
        throw std::runtime_error(yices_error_string());
    return value;
}

term_t unwrap(const SmtTerm &term)
{
    auto value = std::dynamic_pointer_cast<YicesTerm>(term);
    if (!value)
        throw std::runtime_error("expected a Yices SMT term");
    return value->value;
}

SmtTerm wrap(term_t value, type_t type = NULL_TYPE, DagNode *original = nullptr)
{
    return std::make_shared<YicesTerm>(checked(value), type, original);
}
}

SmtTerm YicesModel::get(SmtTerm key)
{
    auto found = values.find(unwrap(key));
    if (found == values.end()) return nullptr;
    return wrap(found->second.first, found->second.second);
}

SmtTermVector YicesModel::keys()
{
    auto result = std::make_shared<std::vector<SmtTerm>>();
    for (const auto &entry : values)
        result->push_back(wrap(entry.first));
    return result;
}

YicesConverter::YicesConverter(const SMT_Info &info)
    : NativeSmtConverter(info)
{
    static std::once_flag initialized;
    std::call_once(initialized, [] { yices_init(); });
}

void YicesConverter::prepareFor(VisibleModule *module)
{
    sg.setModule(module);
    smtManagerVariableMap.clear();
    reverseCache.clear();
}

void YicesConverter::markReachableNodes()
{
    for (const auto &entry : smtManagerVariableMap)
        entry.first->mark();
}

DagNode *YicesConverter::remember(term_t value, DagNode *dag)
{
    if (reverseCache.find(value) == reverseCache.end())
        reverseCache.emplace(value, std::make_shared<RootedDag>(dag));
    return dag;
}

SmtTerm YicesConverter::dag2term(DagNode *dag)
{
    term_t value = convert(dag);
    remember(value, dag);
    return wrap(value, yices_type_of_term(value), dag);
}

DagHandle YicesConverter::term2dag(SmtTerm term)
{
    auto value = std::dynamic_pointer_cast<YicesTerm>(term);
    if (!value) throw std::runtime_error("expected a Yices SMT term");
    DagNode *dag = value->original ? value->original->get() : convertBack(value->value, value->type);
    DagHandle result(dag);
    if (dag->getSort() == nullptr)
    {
        auto *context = new UserLevelRewritingContext(dag);
        dag->computeTrueSort(*context);
        delete context;
    }
    return result;
}

term_t YicesConverter::makeVariable(DagNode *dag)
{
    auto found = smtManagerVariableMap.find(dag);
    if (found != smtManagerVariableMap.end()) return found->second;
    for (const auto &entry : smtManagerVariableMap)
    {
        if (dag->equal(entry.first))
        {
            smtManagerVariableMap.emplace(dag, entry.second);
            return entry.second;
        }
    }

    type_t type = NULL_TYPE;
    if (auto *variable = dynamic_cast<VariableDagNode *>(dag))
    {
        switch (smtInfo.getType(variable->symbol()->getRangeSort()))
        {
        case SMT_Info::BOOLEAN: type = yices_bool_type(); break;
        case SMT_Info::INTEGER: type = yices_int_type(); break;
        case SMT_Info::REAL: type = yices_real_type(); break;
        default: throw std::runtime_error("variable is not an SMT sort");
        }
    }
    else
    {
        Sort *idSort = sg.getSort("SMTVarId");
        if (!idSort) throw std::runtime_error("not an SMT variable");
        Vector<ConnectedComponent *> domain;
        domain.push_back(idSort->component());
        const struct { const char *symbol; const char *sort; } forms[] = {
            {"b", "BooleanVar"}, {"i", "IntegerVar"}, {"r", "RealVar"}};
        for (const auto &form : forms)
        {
            Sort *target = sg.getSort(form.sort);
            if (target && dag->symbol() == sg.getSymbol(form.symbol, domain, target->component()))
            {
                if (form.symbol[0] == 'b') type = yices_bool_type();
                if (form.symbol[0] == 'i') type = yices_int_type();
                if (form.symbol[0] == 'r') type = yices_real_type();
                break;
            }
        }
        if (type == NULL_TYPE) throw std::runtime_error("not an SMT variable");
    }
    term_t result = checked(yices_new_uninterpreted_term(type));
    smtManagerVariableMap.emplace(dag, result);
    return result;
}

term_t YicesConverter::convert(DagNode *dag)
{
    if (auto *number = dynamic_cast<SMT_NumberDagNode *>(dag))
        return checked(yices_mpq(number->getValue().get_mpq_t()));
    if (dynamic_cast<VariableDagNode *>(dag)) return makeVariable(dag);
    try { return makeVariable(dag); }
    catch (const std::runtime_error &) { /* ordinary SMT operator */ }

    auto *symbol = dynamic_cast<SMT_Symbol *>(dag->symbol());
    if (!symbol) throw std::runtime_error("term is not an SMT expression");
    std::vector<term_t> args;
    if (symbol->arity())
    {
        auto *freeDag = dynamic_cast<FreeDagNode *>(dag);
        if (!freeDag) throw std::runtime_error("SMT operator has no free arguments");
        for (int i = 0; i < symbol->arity(); ++i)
            args.push_back(convert(freeDag->getArgument(i)));
    }
    term_t result = NULL_TERM;
    switch (symbol->getOperator())
    {
    case SMT_Symbol::CONST_TRUE: result = yices_true(); break;
    case SMT_Symbol::CONST_FALSE: result = yices_false(); break;
    case SMT_Symbol::NOT: result = yices_not(args[0]); break;
    case SMT_Symbol::AND: result = yices_and2(args[0], args[1]); break;
    case SMT_Symbol::OR: result = yices_or2(args[0], args[1]); break;
    case SMT_Symbol::XOR: result = yices_xor2(args[0], args[1]); break;
    case SMT_Symbol::IMPLIES: result = yices_implies(args[0], args[1]); break;
    case SMT_Symbol::EQUALS: result = yices_eq(args[0], args[1]); break;
    case SMT_Symbol::NOT_EQUALS: result = yices_not(yices_eq(args[0], args[1])); break;
    case SMT_Symbol::ITE: result = yices_ite(args[0], args[1], args[2]); break;
    case SMT_Symbol::UNARY_MINUS: result = yices_neg(args[0]); break;
    case SMT_Symbol::MINUS: result = yices_sub(args[0], args[1]); break;
    case SMT_Symbol::PLUS: result = yices_add(args[0], args[1]); break;
    case SMT_Symbol::MULT: result = yices_mul(args[0], args[1]); break;
    case SMT_Symbol::DIV: result = yices_idiv(args[0], args[1]); break;
    case SMT_Symbol::MOD: result = yices_imod(args[0], args[1]); break;
    case SMT_Symbol::DIVISIBLE: result = yices_divides_atom(args[1], args[0]); break;
    case SMT_Symbol::REAL_DIVISION: result = yices_division(args[0], args[1]); break;
    case SMT_Symbol::LT: result = yices_arith_lt_atom(args[0], args[1]); break;
    case SMT_Symbol::LEQ: result = yices_arith_leq_atom(args[0], args[1]); break;
    case SMT_Symbol::GT: result = yices_arith_gt_atom(args[0], args[1]); break;
    case SMT_Symbol::GEQ: result = yices_arith_geq_atom(args[0], args[1]); break;
    case SMT_Symbol::TO_REAL: result = args[0]; break;
    case SMT_Symbol::TO_INTEGER: result = yices_floor(args[0]); break;
    case SMT_Symbol::IS_INTEGER: result = yices_is_int_atom(args[0]); break;
    default: throw std::runtime_error("unsupported SMT operator in Yices converter");
    }
    return checked(result);
}

DagNode *YicesConverter::convertBack(term_t value, type_t expectedType)
{
    auto cached = reverseCache.find(value);
    if (cached != reverseCache.end()) return cached->second->get();

    for (const auto &entry : smtManagerVariableMap)
        if (entry.second == value) return remember(value, entry.first);

    int32_t boolean;
    if (yices_bool_const_value(value, &boolean) == 0)
    {
        Vector<ConnectedComponent *> domain;
        auto *sort = sg.getKind("Boolean");
        return remember(value,
            sg.getSymbol(boolean ? "true" : "false", domain, sort)->makeDagNode());
    }
    mpq_class number;
    if (yices_rational_const_value(value, number.get_mpq_t()) == 0)
    {
        const bool integer = expectedType == yices_int_type();
        Vector<ConnectedComponent *> domain;
        auto *sort = sg.getKind(integer ? "Integer" : "Real");
        auto *symbol = static_cast<SMT_NumberSymbol *>(sg.getSymbol(
            integer ? "<Integers>" : "<Reals>", domain, sort));
        return remember(value, new SMT_NumberDagNode(symbol, number));
    }

    const auto constructor = yices_term_constructor(value);
    const auto boolType = yices_bool_type();
    auto kindOf = [this](type_t type) -> ConnectedComponent *
    {
        if (type == yices_bool_type()) return sg.getKind("Boolean");
        if (type == yices_int_type()) return sg.getKind("Integer");
        if (type == yices_real_type()) return sg.getKind("Real");
        throw std::runtime_error("unsupported Yices term type in Maude conversion");
    };
    auto foldBoolean = [&](const char *name, const std::vector<term_t> &children) -> DagNode *
    {
        if (children.empty())
            throw std::runtime_error("Yices Boolean term has no children");
        DagNode *result = convertBack(children[0], boolType);
        if (children.size() == 1) return remember(value, result);
        auto *sort = sg.getKind("Boolean");
        Vector<ConnectedComponent *> domain;
        domain.push_back(sort);
        domain.push_back(sort);
        Symbol *symbol = sg.getSymbol(name, domain, sort);
        std::vector<std::shared_ptr<RootedDag>> intermediateRoots;
        for (size_t i = 1; i < children.size(); ++i)
        {
            Vector<DagNode *> arguments(2);
            arguments[0] = result;
            arguments[1] = convertBack(children[i], boolType);
            result = symbol->makeDagNode(arguments);
            intermediateRoots.push_back(std::make_shared<RootedDag>(result));
        }
        return remember(value, result);
    };

    if (constructor == YICES_NOT_TERM)
    {
        term_t child = checked(yices_term_child(value, 0));
        if (yices_term_constructor(child) == YICES_OR_TERM)
        {
            // Yices stores conjunctions as NOT(OR(...)); negating each
            // disjunct recovers the operands, including normalized atoms.
            const int32_t count = yices_term_num_children(child);
            std::vector<term_t> conjuncts;
            for (int32_t i = 0; i < count; ++i)
            {
                term_t disjunct = checked(yices_term_child(child, i));
                conjuncts.push_back(checked(yices_not(disjunct)));
            }
            if (conjuncts.size() >= 2)
                return foldBoolean("_and_", conjuncts);
        }
        Vector<ConnectedComponent *> domain;
        auto *sort = sg.getKind("Boolean");
        domain.push_back(sort);
        Vector<DagNode *> argument(1);
        argument[0] = convertBack(child, boolType);
        return remember(value, sg.getSymbol("not_", domain, sort)->makeDagNode(argument));
    }
    if (yices_type_of_term(value) == boolType)
    {
        term_t complement = checked(yices_not(value));
        auto opposite = reverseCache.find(complement);
        if (opposite != reverseCache.end())
        {
            Vector<ConnectedComponent *> domain;
            auto *sort = sg.getKind("Boolean");
            domain.push_back(sort);
            Vector<DagNode *> argument(1);
            argument[0] = opposite->second->get();
            return remember(value, sg.getSymbol("not_", domain, sort)->makeDagNode(argument));
        }
    }
    if (constructor == YICES_OR_TERM || constructor == YICES_XOR_TERM)
    {
        const int32_t count = yices_term_num_children(value);
        std::vector<term_t> children;
        for (int32_t i = 0; i < count; ++i)
            children.push_back(checked(yices_term_child(value, i)));
        return foldBoolean(constructor == YICES_OR_TERM ? "_or_" : "_xor_", children);
    }
    if (constructor == YICES_EQ_TERM)
    {
        term_t left = checked(yices_term_child(value, 0));
        term_t right = checked(yices_term_child(value, 1));
        auto *sort = kindOf(yices_type_of_term(left));
        Vector<ConnectedComponent *> domain;
        domain.push_back(sort);
        domain.push_back(sort);
        Vector<DagNode *> arguments(2);
        arguments[0] = convertBack(left, yices_type_of_term(left));
        arguments[1] = convertBack(right, yices_type_of_term(right));
        return remember(value,
            sg.getSymbol("_===_", domain, sg.getKind("Boolean"))->makeDagNode(arguments));
    }
    if (constructor == YICES_ITE_TERM)
    {
        term_t condition = checked(yices_term_child(value, 0));
        term_t positive = checked(yices_term_child(value, 1));
        term_t negative = checked(yices_term_child(value, 2));
        auto *sort = kindOf(expectedType == NULL_TYPE ? yices_type_of_term(value) : expectedType);
        Vector<ConnectedComponent *> domain;
        domain.push_back(sg.getKind("Boolean"));
        domain.push_back(sort);
        domain.push_back(sort);
        Vector<DagNode *> arguments(3);
        arguments[0] = convertBack(condition, boolType);
        arguments[1] = convertBack(positive, yices_type_of_term(positive));
        arguments[2] = convertBack(negative, yices_type_of_term(negative));
        return remember(value, sg.getSymbol("_?_:_", domain, sort)->makeDagNode(arguments));
    }

    char *description = yices_term_to_string(value, 120, 10, 0);
    std::string message = "cannot convert Yices term to Maude DAG: ";
    message += description ? description : yices_error_string();
    yices_free_string(description);
    throw std::runtime_error(message);
}

YicesConnector::YicesConnector(std::shared_ptr<YicesConverter> converter)
    : conv(std::move(converter)), context(yices_new_context(nullptr))
{
    if (!context) throw std::runtime_error(yices_error_string());
}

YicesConnector::~YicesConnector() { yices_free_context(context); }

SmtResult YicesConnector::check_sat(SmtTermVector constraints)
{
    for (const auto &constraint : *constraints)
        if (yices_assert_formula(context, unwrap(constraint)) < 0)
        {
            IssueWarning("Yices assertion failed: " << yices_error_string());
            return unknown;
        }
    switch (yices_check_context(context, nullptr))
    {
    case STATUS_SAT: return sat;
    case STATUS_UNSAT: return unsat;
    default:
        IssueWarning("Yices returned unknown satisfiability: " << yices_error_string());
        return unknown;
    }
}

SmtTerm YicesConnector::add_const(SmtTerm accumulated, SmtTerm current)
{
    if (!accumulated) return current;
    return wrap(yices_and2(unwrap(accumulated), unwrap(current)), yices_bool_type());
}

TermSubst YicesConnector::mk_subst(std::map<DagNode *, DagNode *> &substitution)
{
    auto result = std::make_shared<YicesSubstitution>();
    for (const auto &entry : substitution)
    {
        result->from.push_back(unwrap(conv->dag2term(entry.first)));
        result->to.push_back(unwrap(conv->dag2term(entry.second)));
    }
    return result;
}

bool YicesConnector::subsume(TermSubst substitution, SmtTerm previous,
                              SmtTerm accumulated, SmtTerm current)
{
    auto subst = std::dynamic_pointer_cast<YicesSubstitution>(substitution);
    if (!subst) throw std::runtime_error("invalid Yices substitution");
    term_t conclusion = checked(yices_subst_term(subst->from.size(),
        subst->from.data(), subst->to.data(), unwrap(previous)));
    term_t premise = checked(yices_and2(unwrap(accumulated), unwrap(current)));
    term_t counterexample = checked(yices_not(yices_implies(premise, conclusion)));
    push();
    try
    {
        if (yices_assert_formula(context, counterexample) < 0)
            throw std::runtime_error(yices_error_string());
        smt_status_t result = yices_check_context(context, nullptr);
        pop();
        if (result == STATUS_UNSAT) return true;
        if (result == STATUS_SAT) return false;
        IssueWarning("Yices returned unknown during subsumption");
        return false;
    }
    catch (...)
    {
        pop();
        throw;
    }
}

SmtModel YicesConnector::get_model()
{
    model_t *model = yices_get_model(context, 1);
    if (!model) throw std::runtime_error(yices_error_string());
    std::map<term_t, std::pair<term_t, type_t>> values;
    for (const auto &entry : conv->variables())
    {
        term_t value = yices_get_value_as_term(model, entry.second);
        if (value != NULL_TERM)
            values.emplace(entry.second,
                           std::make_pair(value, yices_type_of_term(entry.second)));
    }
    yices_free_model(model);
    return std::make_shared<YicesModel>(std::move(values));
}

void YicesConnector::push()
{
    if (yices_push(context) < 0) throw std::runtime_error(yices_error_string());
    ++pushCount;
}

void YicesConnector::pop()
{
    if (!pushCount || yices_pop(context) < 0)
        throw std::runtime_error("unbalanced Yices pop");
    --pushCount;
}

void YicesConnector::reset()
{
    yices_reset_context(context);
    pushCount = 0;
}

void YicesConnector::set_logic(const char *requested)
{
    if (logic == requested) return;
    if (!logic.empty())
        throw std::runtime_error("Yices logic cannot be changed after initialization");
    ctx_config_t *config = yices_new_config();
    if (!config) throw std::runtime_error(yices_error_string());
    if (yices_default_config_for_logic(config, requested) < 0)
    {
        yices_free_config(config);
        throw std::runtime_error(yices_error_string());
    }
    context_t *replacement = yices_new_context(config);
    yices_free_config(config);
    if (!replacement) throw std::runtime_error(yices_error_string());
    yices_free_context(context);
    context = replacement;
    logic = requested;
    pushCount = 0;
}
