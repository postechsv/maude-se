// utility stuff
#include "macros.hh"
#include "vector.hh"

// forward declarations
#include "interface.hh"
#include "core.hh"
#include "freeTheory.hh"
#include "variable.hh"
#include "builtIn.hh"
#include "strategyLanguage.hh"
#include "mixfix.hh"

// interface class definitions
#include "symbol.hh"
#include "term.hh"

// core class definitions
#include "rewritingContext.hh"
#include "symbolMap.hh"

// variable class definitions
#include "variableSymbol.hh"
#include "variableDagNode.hh"

// variable class definitions
#include "variableSymbol.hh"
#include "variableDagNode.hh"
#include "variableTerm.hh"

// free theory class definitions
#include "freeDagNode.hh"

// builtin class definition
#include "bindingMacros.hh"

// SMT stuff
#include "SMT_Info.hh"
#include "SMT_Symbol.hh"
#include "SMT_NumberSymbol.hh"
#include "SMT_NumberDagNode.hh"

// front end class definitions
#include "token.hh"

#include "mixfixModule.hh"

// Qid
#include "quotedIdentifierSymbol.hh"
#include "quotedIdentifierDagNode.hh"

#include "smtOpSymbol.hh"
#include "smtCheck.cc"

SmtOpSymbol::SmtOpSymbol(int id, int nrArgs, const Vector<int> &strategy)
    : FreeSymbol(id, nrArgs, strategy)
{
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
    SymbolName = 0;
#include "smtSignature.cc"
#undef MACRO
}

bool SmtOpSymbol::attachData(const Vector<Sort *> &opDeclaration,
                             const char *purpose,
                             const Vector<const char *> &data)
{
    if (data.length() == 1)
    {
        const char *opName = data[0];
        // cout << "opName : " << opName << endl;
#define MACRO(SymbolName, NrArgs)                              \
    if (arity() == NrArgs && strcmp(opName, #SymbolName) == 0) \
        function = &SmtOpSymbol::SymbolName;                   \
    else
#include "smtOpSignature.cc"
#undef MACRO
        return FreeSymbol::attachData(opDeclaration, purpose, data);
        return true;
    }
    return FreeSymbol::attachData(opDeclaration, purpose, data);
}

bool SmtOpSymbol::attachSymbol(const char *purpose, Symbol *symbol)
{
    Assert(symbol != 0, "null symbol for " << purpose);
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
    if (strcmp(purpose, #SymbolName) == 0)                    \
        SymbolName = static_cast<SymbolClass *>(symbol);      \
    else
#include "smtSignature.cc"
#undef MACRO
    {
        IssueWarning("unrecognized symbol hook name " << QUOTE(purpose) << '.');
        return false;
    }
    return true;
}

bool SmtOpSymbol::attachTerm(const char *purpose, Term *term)
{
    BIND_TERM(purpose, term, trueTerm);
    BIND_TERM(purpose, term, falseTerm);
    return FreeSymbol::attachTerm(purpose, term);
}

void SmtOpSymbol::copyAttachments(Symbol *original, SymbolMap *map)
{
    // cout << "copy attachment called" << endl;
    SmtOpSymbol *orig = safeCast(SmtOpSymbol *, original);
    function = orig->function;
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
    COPY_SYMBOL(orig, SymbolName, map, SymbolClass *)
#include "smtSignature.cc"
#undef MACRO

    COPY_TERM(orig, trueTerm, map);
    COPY_TERM(orig, falseTerm, map);

    FreeSymbol::copyAttachments(original, map);
}

void SmtOpSymbol::getDataAttachments(const Vector<Sort *> &opDeclaration,
                                     Vector<const char *> &purposes,
                                     Vector<Vector<const char *>> &data)
{
    FreeSymbol::getDataAttachments(opDeclaration, purposes, data);
}

void SmtOpSymbol::getSymbolAttachments(Vector<const char *> &purposes,
                                       Vector<Symbol *> &symbols)
{
    FreeSymbol::getSymbolAttachments(purposes, symbols);
}

void SmtOpSymbol::getTermAttachments(Vector<const char *> &purposes,
                                     Vector<Term *> &terms)
{
    APPEND_TERM(purposes, terms, trueTerm);
    APPEND_TERM(purposes, terms, falseTerm);
    FreeSymbol::getTermAttachments(purposes, terms);
}

void SmtOpSymbol::postInterSymbolPass()
{
    PREPARE_TERM(trueTerm);
    PREPARE_TERM(falseTerm);
}

void SmtOpSymbol::reset()
{
    trueTerm.reset();
    falseTerm.reset();
}

bool SmtOpSymbol::eqRewrite(DagNode *subject, RewritingContext &context)
{
    Assert(this == subject->symbol(), "bad symbol");

    FreeDagNode *d = safeCast(FreeDagNode *, subject);
    if (standardStrategy())
    {
        const int nrArgs = arity();
        for (int i = 0; i < nrArgs; ++i)
        {
            d->getArgument(i)->reduce(context);
        }
        return (this->*function)(d, context) || FreeSymbol::eqRewrite(subject, context);
    }
    return false;
}

const char *SmtOpSymbol::getLogic(DagNode *dag, SymbolGetter *sg)
{
    Vector<ConnectedComponent *> dom;
    if (dag->symbol() == sg->getSymbol("auto", dom, sg->getKind("Logic")))
    {
        return nullptr;
    }
    if (QuotedIdentifierDagNode *d = static_cast<QuotedIdentifierDagNode *>(dag))
    {
        int id = Token::unBackQuoteSpecials(d->getIdIndex());
        return Token::name(id);
    }
    return nullptr;
}