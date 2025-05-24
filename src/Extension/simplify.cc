#include "SMT_Info.hh"
// #include "variableGenerator.hh"
#include "smtManager.hh"
// #include "smtManagerFactory.hh"
#include "extGlobal.hh"

bool SmtOpSymbol::simplify(FreeDagNode *subject, RewritingContext &context)
{
    if (VisibleModule *m = safeCast(VisibleModule *, this->getModule()))
    {
        const SMT_Info &smtInfo = m->getSMT_Info();

        VariableGenerator vg(smtInfo, true);

        Converter cv = vg.getConverter();
        Connector cn = vg.getConnector();
        cv->prepareFor(m);

        FreeDagNode *f = safeCast(FreeDagNode *, subject);
        RewritingContext *newContext = context.makeSubcontext((f->getArgument(0)));
        newContext->reduce();
        context.addInCount(*newContext);
        DagNode *dag = newContext->root();

        SmtTerm term = cn->simplify(cv->dag2term(dag));

        return context.builtInReplace(subject, cv->term2dag(term));
    }
    return false;
}
