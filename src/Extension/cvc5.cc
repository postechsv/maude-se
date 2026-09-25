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
#include "cvc5.hh"
#include "userLevelRewritingContext.hh"
#include <sstream>
#include <stdexcept>

namespace
{
cvc5::Term unwrap(const SmtTerm &term)
{
    auto wrapped = std::dynamic_pointer_cast<Cvc5Term>(term);
    if (!wrapped)
        throw std::runtime_error("expected a cvc5 SMT term");
    return wrapped->value;
}

SmtTerm wrap(const cvc5::Term &term)
{
    return std::make_shared<Cvc5Term>(term);
}
}

SmtTerm Cvc5Model::get(SmtTerm key)
{
    auto found = values.find(unwrap(key));
    return found == values.end() ? nullptr : wrap(found->second);
}

SmtTermVector Cvc5Model::keys()
{
    auto result = std::make_shared<std::vector<SmtTerm>>();
    for (const auto &entry : values)
        result->push_back(wrap(entry.first));
    return result;
}

Cvc5Converter::Cvc5Converter(const SMT_Info &info)
    : NativeSmtConverter(info) {}

void Cvc5Converter::prepareFor(VisibleModule *module)
{
    clearConversionCache();
    sg.setModule(module);
    smtManagerVariableMap.clear();
}

void Cvc5Converter::markReachableNodes()
{
    for (const auto &entry : smtManagerVariableMap)
        entry.first->mark();
}

SmtTerm Cvc5Converter::dag2term(DagNode *dag)
{
    return wrap(convert(dag));
}

DagHandle Cvc5Converter::term2dag(SmtTerm term)
{
    DagRootFrame frame;
    ActiveDagRootFrame active(activeFrame, frame);
    DagNode *dag = convertBack(unwrap(term));
    if (dag->getSort() == nullptr)
    {
        auto *context = new UserLevelRewritingContext(dag);
        dag->computeTrueSort(*context);
        delete context;
    }
    return DagHandle(dag);
}

cvc5::Term Cvc5Converter::makeVariable(DagNode *dag)
{
    auto found = smtManagerVariableMap.find(dag);
    if (found != smtManagerVariableMap.end())
        return found->second;
    for (const auto &entry : smtManagerVariableMap)
    {
        if (dag->equal(entry.first))
        {
            smtManagerVariableMap.emplace(dag, entry.second);
            return entry.second;
        }
    }

    cvc5::Sort sort;
    std::string name;
    if (auto *variable = dynamic_cast<VariableDagNode *>(dag))
    {
        name = Token::name(variable->id());
        switch (smtInfo.getType(variable->symbol()->getRangeSort()))
        {
        case SMT_Info::BOOLEAN:
            sort = tm.getBooleanSort(); name += "_Boolean"; break;
        case SMT_Info::INTEGER:
            sort = tm.getIntegerSort(); name += "_Integer"; break;
        case SMT_Info::REAL:
            sort = tm.getRealSort(); name += "_Real"; break;
        default:
            throw std::runtime_error("variable is not an SMT sort");
        }
    }
    else
    {
        Sort *idSort = sg.getSort("SMTVarId");
        if (!idSort)
            throw std::runtime_error("not an SMT variable");
        Vector<ConnectedComponent *> domain;
        domain.push_back(idSort->component());
        const struct { const char *symbol; const char *sort; } forms[] = {
            {"b", "BooleanVar"}, {"i", "IntegerVar"}, {"r", "RealVar"}};
        for (const auto &form : forms)
        {
            Sort *target = sg.getSort(form.sort);
            if (target && dag->symbol() == sg.getSymbol(form.symbol, domain, target->component()))
            {
                if (form.symbol[0] == 'b') sort = tm.getBooleanSort();
                if (form.symbol[0] == 'i') sort = tm.getIntegerSort();
                if (form.symbol[0] == 'r') sort = tm.getRealSort();
                std::ostringstream stream;
                stream << form.symbol << '_' << static_cast<const void *>(dag);
                name = stream.str();
                break;
            }
        }
        if (sort.isNull())
            throw std::runtime_error("not an SMT variable");
    }

    cvc5::Term result = tm.mkConst(sort, name);
    smtManagerVariableMap.emplace(dag, result);
    return result;
}

cvc5::Term Cvc5Converter::convert(DagNode *dag)
{
    if (auto cached = cachedConversion(dag)) return *cached;
    cvc5::Term result = convertUncached(dag);
    rememberConversion(dag, result);
    return result;
}

cvc5::Term Cvc5Converter::convertUncached(DagNode *dag)
{
    if (auto *number = dynamic_cast<SMT_NumberDagNode *>(dag))
    {
        const std::string value = number->getValue().get_str();
        switch (smtInfo.getType(number->symbol()->getRangeSort()))
        {
        case SMT_Info::INTEGER: return tm.mkInteger(value);
        case SMT_Info::REAL: return tm.mkReal(value);
        default: break;
        }
    }
    if (dynamic_cast<VariableDagNode *>(dag))
        return makeVariable(dag);
    try { return makeVariable(dag); }
    catch (const std::runtime_error &) { /* ordinary SMT operator */ }

    auto *symbol = dynamic_cast<SMT_Symbol *>(dag->symbol());
    if (!symbol)
        throw std::runtime_error("term is not an SMT expression");
    std::vector<cvc5::Term> args;
    if (symbol->arity())
    {
        auto *freeDag = dynamic_cast<FreeDagNode *>(dag);
        if (!freeDag)
            throw std::runtime_error("SMT operator has no free arguments");
        for (int i = 0; i < symbol->arity(); ++i)
            args.push_back(convert(freeDag->getArgument(i)));
    }
    using K = cvc5::Kind;
    switch (symbol->getOperator())
    {
    case SMT_Symbol::CONST_TRUE: return tm.mkBoolean(true);
    case SMT_Symbol::CONST_FALSE: return tm.mkBoolean(false);
    case SMT_Symbol::NOT: return tm.mkTerm(K::NOT, args);
    case SMT_Symbol::AND: return tm.mkTerm(K::AND, args);
    case SMT_Symbol::OR: return tm.mkTerm(K::OR, args);
    case SMT_Symbol::XOR: return tm.mkTerm(K::XOR, args);
    case SMT_Symbol::IMPLIES: return tm.mkTerm(K::IMPLIES, args);
    case SMT_Symbol::EQUALS: return tm.mkTerm(K::EQUAL, args);
    case SMT_Symbol::NOT_EQUALS:
        return tm.mkTerm(K::NOT, {tm.mkTerm(K::EQUAL, args)});
    case SMT_Symbol::ITE: return tm.mkTerm(K::ITE, args);
    case SMT_Symbol::UNARY_MINUS: return tm.mkTerm(K::NEG, args);
    case SMT_Symbol::MINUS: return tm.mkTerm(K::SUB, args);
    case SMT_Symbol::PLUS: return tm.mkTerm(K::ADD, args);
    case SMT_Symbol::MULT: return tm.mkTerm(K::MULT, args);
    case SMT_Symbol::DIV:
        return tm.mkTerm(K::INTS_DIVISION, args);
    case SMT_Symbol::MOD:
        return tm.mkTerm(K::INTS_MODULUS, args);
    case SMT_Symbol::REAL_DIVISION:
        return tm.mkTerm(K::DIVISION, args);
    case SMT_Symbol::LT: return tm.mkTerm(K::LT, args);
    case SMT_Symbol::LEQ: return tm.mkTerm(K::LEQ, args);
    case SMT_Symbol::GT: return tm.mkTerm(K::GT, args);
    case SMT_Symbol::GEQ: return tm.mkTerm(K::GEQ, args);
    case SMT_Symbol::TO_REAL: return tm.mkTerm(K::TO_REAL, args);
    case SMT_Symbol::TO_INTEGER: return tm.mkTerm(K::TO_INTEGER, args);
    case SMT_Symbol::IS_INTEGER: return tm.mkTerm(K::IS_INTEGER, args);
    default: throw std::runtime_error("unsupported SMT operator in cvc5 converter");
    }
}

DagNode *Cvc5Converter::convertBack(const cvc5::Term &term)
{
    return activeFrame->keep(convertBackUnrooted(term));
}

DagNode *Cvc5Converter::convertBackUnrooted(const cvc5::Term &term)
{
    for (const auto &entry : smtManagerVariableMap)
        if (entry.second == term)
            return entry.first;

    if (term.isBooleanValue())
    {
        Vector<ConnectedComponent *> domain;
        auto *sort = sg.getKind("Boolean");
        return sg.getSymbol(term.getBooleanValue() ? "true" : "false", domain, sort)->makeDagNode();
    }
    if (term.isIntegerValue() || term.isRealValue())
    {
        const bool integer = term.getSort().isInteger();
        auto *sort = sg.getKind(integer ? "Integer" : "Real");
        Vector<ConnectedComponent *> domain;
        auto *symbol = static_cast<SMT_NumberSymbol *>(
            sg.getSymbol(integer ? "<Integers>" : "<Reals>", domain, sort));
        mpq_class value(integer ? term.getIntegerValue() : term.getRealValue());
        return new SMT_NumberDagNode(symbol, value);
    }

    const cvc5::Kind kind = term.getKind();
    const char *name = nullptr;
    switch (kind)
    {
    case cvc5::Kind::NOT: name = "not_"; break;
    case cvc5::Kind::AND: name = "_and_"; break;
    case cvc5::Kind::OR: name = "_or_"; break;
    case cvc5::Kind::XOR: name = "_xor_"; break;
    case cvc5::Kind::IMPLIES: name = "_implies_"; break;
    case cvc5::Kind::EQUAL: name = "_===_"; break;
    case cvc5::Kind::ITE: name = "_?_:_"; break;
    case cvc5::Kind::NEG: name = "-_"; break;
    case cvc5::Kind::ADD: name = "_+_"; break;
    case cvc5::Kind::SUB: name = "_-_"; break;
    case cvc5::Kind::MULT: name = "_*_"; break;
    case cvc5::Kind::INTS_DIVISION: name = "_div_"; break;
    case cvc5::Kind::INTS_MODULUS: name = "_mod_"; break;
    case cvc5::Kind::DIVISION: name = "_/_"; break;
    case cvc5::Kind::LT: name = "_<_"; break;
    case cvc5::Kind::LEQ: name = "_<=_"; break;
    case cvc5::Kind::GT: name = "_>_"; break;
    case cvc5::Kind::GEQ: name = "_>=_"; break;
    case cvc5::Kind::TO_INTEGER: name = "toInteger"; break;
    case cvc5::Kind::TO_REAL: name = "toReal"; break;
    case cvc5::Kind::IS_INTEGER: name = "isInteger"; break;
    default: throw std::runtime_error("cannot convert cvc5 term to Maude DAG");
    }
    Vector<DagNode *> arguments(term.getNumChildren());
    Vector<ConnectedComponent *> domain;
    for (size_t i = 0; i < term.getNumChildren(); ++i)
    {
        auto child = term[i];
        arguments[i] = convertBack(child);
        const auto sort = child.getSort();
        domain.push_back(sg.getKind(sort.isBoolean() ? "Boolean" :
                                    sort.isInteger() ? "Integer" : "Real"));
    }
    const auto sort = term.getSort();
    auto *range = sg.getKind(sort.isBoolean() ? "Boolean" :
                             sort.isInteger() ? "Integer" : "Real");
    if (arguments.length() > 2 &&
        (kind == cvc5::Kind::AND || kind == cvc5::Kind::OR ||
         kind == cvc5::Kind::XOR || kind == cvc5::Kind::ADD ||
         kind == cvc5::Kind::MULT))
    {
        Vector<ConnectedComponent *> binaryDomain;
        binaryDomain.push_back(range);
        binaryDomain.push_back(range);
        Symbol *binary = sg.getSymbol(name, binaryDomain, range);
        DagNode *result = arguments[0];
        for (int i = 1; i < arguments.length(); ++i)
        {
            Vector<DagNode *> pair(2);
            pair[0] = result;
            pair[1] = arguments[i];
            result = binary->makeDagNode(pair);
        }
        return result;
    }
    return sg.getSymbol(name, domain, range)->makeDagNode(arguments);
}

Cvc5Connector::Cvc5Connector(std::shared_ptr<Cvc5Converter> converter)
    : tm(converter->manager()), conv(std::move(converter)), solver(tm)
{
    solver.setOption("produce-models", "true");
    solver.setOption("incremental", "true");
}

SmtResult Cvc5Connector::check_sat(SmtTermVector constraints)
{
    for (const auto &constraint : *constraints)
        solver.assertFormula(unwrap(constraint));
    const auto result = solver.checkSat();
    if (result.isSat()) return sat;
    if (result.isUnsat()) return unsat;
    IssueWarning("cvc5 returned unknown satisfiability");
    return unknown;
}

SmtTerm Cvc5Connector::add_const(SmtTerm accumulated, SmtTerm current)
{
    return accumulated ? wrap(tm.mkTerm(cvc5::Kind::AND,
                                       {unwrap(accumulated), unwrap(current)})) : current;
}

TermSubst Cvc5Connector::mk_subst(std::map<DagNode *, DagNode *> &substitution)
{
    auto result = std::make_shared<Cvc5Substitution>();
    for (const auto &entry : substitution)
    {
        result->from.push_back(unwrap(conv->dag2term(entry.first)));
        result->to.push_back(unwrap(conv->dag2term(entry.second)));
    }
    return result;
}

bool Cvc5Connector::subsume(TermSubst substitution, SmtTerm previous,
                             SmtTerm accumulated, SmtTerm current)
{
    auto subst = std::dynamic_pointer_cast<Cvc5Substitution>(substitution);
    if (!subst) throw std::runtime_error("invalid cvc5 substitution");
    const auto premise = tm.mkTerm(cvc5::Kind::AND,
                                   {unwrap(accumulated), unwrap(current)});
    const auto conclusion = unwrap(previous).substitute(subst->from, subst->to);
    const auto counterexample = tm.mkTerm(cvc5::Kind::NOT,
        {tm.mkTerm(cvc5::Kind::IMPLIES, {premise, conclusion})});
    solver.push();
    try
    {
        solver.assertFormula(counterexample);
        const auto result = solver.checkSat();
        solver.pop();
        if (result.isUnsat()) return true;
        if (result.isSat()) return false;
        IssueWarning("cvc5 returned unknown during subsumption");
        return false;
    }
    catch (...)
    {
        solver.pop();
        throw;
    }
}

SmtModel Cvc5Connector::get_model()
{
    std::map<cvc5::Term, cvc5::Term> values;
    for (const auto &entry : conv->variables())
        values.emplace(entry.second, solver.getValue(entry.second));
    return std::make_shared<Cvc5Model>(std::move(values));
}

void Cvc5Connector::push() { solver.push(); ++pushCount; }
void Cvc5Connector::pop()
{
    if (!pushCount) throw std::runtime_error("unbalanced cvc5 pop");
    solver.pop(); --pushCount;
}
void Cvc5Connector::reset() { solver.resetAssertions(); pushCount = 0; }
SmtTerm Cvc5Connector::simplify(SmtTerm term) { return wrap(solver.simplify(unwrap(term))); }

void Cvc5Connector::set_logic(const char *requested)
{
    if (logic == requested) return;
    if (!logic.empty())
        throw std::runtime_error("cvc5 logic cannot be changed after initialization");
    solver.setLogic(requested);
    logic = requested;
}
