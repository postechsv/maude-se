#ifndef MAUDE_SE_REWRITE_SMT_SEARCH_FACTORY_HH
#define MAUDE_SE_REWRITE_SMT_SEARCH_FACTORY_HH

#include "freshVariableSource.hh"
#include "pattern.hh"
#include "rewriteSmtSequenceSearch.hh"
#include "smtManager.hh"
#include "visibleModule.hh"

// The meta-level operator and the Python term API must initialize the solver
// in the same way. Ownership of context and patterns passes to the search.
inline RewriteSmtSequenceSearch *makeRewriteSmtSearch(
    VisibleModule *module, RewritingContext *context,
    RewriteSmtSequenceSearch::SearchType type, Pattern *goal,
    Pattern *smtGoal, const char *logic, bool fold, bool merge,
    int depth, const mpz_class &avoidVariableNumber = 0)
{
    const SMT_Info &info = module->getSMT_Info();
    VariableGenerator *engine = new VariableGenerator(info, false, true);
    Converter converter = engine->getConverter();
    Connector connector = engine->getConnector();
    Connector foldingConnector = engine->getConnector2();
    if (logic)
    {
        connector->set_logic(logic);
        foldingConnector->set_logic(logic);
    }
    converter->prepareFor(module);
    return new RewriteSmtSequenceSearch(
        context, type, goal, smtGoal, info, engine,
        new FreshVariableSource(module), fold, merge, depth,
        avoidVariableNumber);
}

#endif
