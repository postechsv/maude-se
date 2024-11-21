#include "SMT_Info.hh"
// #include "variableGenerator.hh"
#include "smtManager.hh"
#include "smtManagerFactory.hh"
#include "rewriteSmtSequenceSearch.hh"
#include "extGlobal.hh"

RewriteSmtSequenceSearch *
MetaLevelSmtOpSymbol::make_RewriteSmtSequenceSearch(MetaModule *m,
                                                    FreeDagNode *subject,
                                                    RewritingContext &context) const
{
    //   op metaSmtSearchInternal : Module Term Term Condition Term Qid Nat Bound Nat Qid Bool Bool -> SmtResult2?
    DagNode *metaVarNumber = subject->getArgument(6);
    RewriteSmtSequenceSearch::SearchType searchType;
    int maxDepth;
    bool fold;
    bool merge;
    if (metaLevel->isNat(metaVarNumber) &&
        metaLevel->downSearchType(subject->getArgument(5), searchType) &&
        searchType != SequenceSearch::NORMAL_FORM &&
        metaLevel->downBound(subject->getArgument(7), maxDepth) && 
        metaLevel->downBool(subject->getArgument(10), fold) && metaLevel->downBool(subject->getArgument(11), merge))
    {
        Term *startTerm;
        Term *target;
        Term *smtGoalTerm = metaLevel->downTerm(subject->getArgument(4), m);
        const char* logic = downLogic(subject->getArgument(9));
        if (metaLevel->downTermPair(subject->getArgument(1), subject->getArgument(2), startTerm, target, m))
        {
            if (m->findSMT_Symbol(target) == 0) // target shouldn't have SMT operators
            {
                VariableInfo variableInfo;
                if (MixfixModule::findNonlinearVariable(target, variableInfo) == 0) // target shouldn't have nonlinear variables
                {
                    Vector<ConditionFragment *> condition;
                    if (metaLevel->downCondition(subject->getArgument(3), m, condition))
                    {
                        m->protect();
                        const mpz_class &varNumber = metaLevel->getNat(metaVarNumber);
                        RewritingContext *startContext = term2RewritingContext(startTerm, context);
                        context.addInCount(*startContext);

                        Pattern *goal = new Pattern(target, false, condition);
                        Pattern *smtGoal = new Pattern(smtGoalTerm, false);
                        const SMT_Info &smtInfo = m->getSMT_Info();
                        // SmtManager *vg = new SmtManager(smtInfo);
                        SmtManager* vg = smtManagerFactory->create(smtInfo);
                        // SmtManagerFactory *factory = new SmtFactory();
                        // WrapperFactory *factory = m->getOwner()->getWrapperFactory();
                        // Converter *conv = factory->createConverter();
                        // Connector *conn = factory->createConnector();

                        Converter *conv = vg->getConverter();
                        Connector *conn = vg->getConnector();
                        // prepare for the current module
                        conv->prepareFor(m);
                        conn->set_logic(logic);
                        // vg->setUnderline(conn, conv);

                        // cout << "   !!! Made cached SMT_RewriteSequenceSearch !!!" << endl;

                        return new RewriteSmtSequenceSearch(startContext, // pass responsibility for deletion
                                                            searchType,
                                                            goal,    // pass responsibility for deletion
                                                            smtGoal, // pass responsibility for deletion
                                                            smtInfo,
                                                            vg, // pass responsibility for deletion
                                                            new FreshVariableSource(m),
                                                            conn, conv,
                                                            fold, merge,
                                                            maxDepth,
                                                            varNumber);
                    }
                }
            }
        }
    }
    return 0;
}

bool MetaLevelSmtOpSymbol::metaSmtSearch(FreeDagNode *subject, RewritingContext &context)
{
    //
    // op metaSmtSearchInternal : Module Term Term Condition Term Qid Nat Bound Nat Qid Bool Bool -> SmtResult2?
    //
    if (MetaModule *m = metaLevel->downModule(subject->getArgument(0)))
    {
        // TODO
        // if (m->validForSMT_Rewriting())
        if (true)
        {
            Int64 solutionNr;
            if (metaLevel->downSaturate64(subject->getArgument(8), solutionNr) &&
                solutionNr >= 0)
            {
                RewriteSmtSequenceSearch *smtState;
                Int64 lastSolutionNr;
                // cout << "searching for " << subject << " with solution # " << solutionNr << endl;
                if (m->getCachedStateObject(subject, context, solutionNr, smtState, lastSolutionNr))
                    m->protect(); // Use cached state
                else if ((smtState = make_RewriteSmtSequenceSearch(m, subject, context)))
                    lastSolutionNr = -1;
                else
                    return false;

                DagNode *result;
                while (lastSolutionNr < solutionNr)
                {
                    // cout << "current smtState : " << smtState << " (run)"<< endl;
                    bool success = smtState->findNextMatch();
                    smtState->transferCountTo(context);
                    if (!success)
                    {
                        delete smtState;
                        result = metaLevel->upSmtFailure();
                        goto fail;
                    }
                    context.incrementRlCount();
                    ++lastSolutionNr;
                }
                m->insert(subject, smtState, solutionNr);
                result = upSmtResult(smtState->getStateDag(smtState->getStateNr()),
                                                *(smtState->getSubstitution()),
                                                // *smtState->getVariableInfo(smtState->getStateNr()),
                                                *(smtState->getVariableInfo()),
                                                smtState->getSMT_VarIndices(),
                                                smtState->getFinalConstraint(),
                                                smtState->getMaxVariableNumber(),
                                                m, 
                                                smtState->getStateModel(smtState->getStateNr()) // should delete this
                                                );
            fail:
                (void)m->unprotect();
                return context.builtInReplace(subject, result);
            }
        }
    }
    return false;
}
