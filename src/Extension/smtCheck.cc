#include "SMT_Info.hh"
// #include "variableGenerator.hh"
#include "smtManager.hh"
// #include "smtManagerFactory.hh"
#include "extGlobal.hh"

// #include <chrono>

bool SmtOpSymbol::smtCheck(FreeDagNode *subject, RewritingContext &context)
{
    if (VisibleModule *m = safeCast(VisibleModule *, this->getModule()))
    {
        SymbolGetter sg;
        sg.setModule(m);

        Vector<ConnectedComponent *> dom;

        // auto start = std::chrono::high_resolution_clock::now();

        Symbol *trueSymbol = sg.getSymbol("true", dom, sg.getKind("Bool"));
        Symbol *falseSymbol = sg.getSymbol("false", dom, sg.getKind("Bool"));

        bool isMakeAssn = subject->getArgument(2)->symbol() == trueSymbol;
        const SMT_Info &smtInfo = m->getSMT_Info();
        const char *logic = getLogic(subject->getArgument(1), &sg);

        // auto mid = std::chrono::high_resolution_clock::now();

        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
        // std::cout << "SmtCheck::Prepare elapsed time : " << duration.count() << " ms\n";

        VariableGenerator vg(smtInfo, false);

        // auto mid1 = std::chrono::high_resolution_clock::now();

        // auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(mid1 - mid);
        // std::cout << "SmtCheck::Init elapsed time : " << duration1.count() << " ms\n";

        vg.getConverter()->prepareFor(m);

        // auto mid2 = std::chrono::high_resolution_clock::now();

        // auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(mid2 - mid1);
        // std::cout << "SmtCheck::ModuleSet elapsed time : " << duration2.count() << " ms\n";

        if (logic)
            vg.getConnector()->set_logic(logic);

        VariableGenerator::Result result = vg.assertDag(subject->getArgument(0));
        // auto mid3 = std::chrono::high_resolution_clock::now();

        // auto duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(mid3 - mid2);
        // std::cout << "SmtCheck::CheckSat elapsed time : " << duration3.count() << " ms\n";

        switch (result)
        {
        case VariableGenerator::BAD_DAG:
        {
            break;
        }
        case VariableGenerator::SAT_UNKNOWN:
        case VariableGenerator::UNSAT:
        case VariableGenerator::SAT:
        {
            DagNode *r;
            if (isMakeAssn && result == VariableGenerator::SAT)
            {
                // This is Hack:
                // we use symbol in the downed module, while we use subject's module symbol to create new expressions.
                // We should upDagNode with different modules.
                // MixfixModule* module = safeCast(MixfixModule*, subject->symbol()->getModule());
                r = make_model(&vg, &sg);
            }
            else if (result == VariableGenerator::SAT_UNKNOWN)
            {
                r = unknownResultSymbol->makeDagNode();
            }
            else
            {
                r = result == VariableGenerator::SAT ? trueSymbol->makeDagNode() : falseSymbol->makeDagNode();
            }
            return context.builtInReplace(subject, r);
        }
        }
        return false;
    }
    return false;
}

DagNode *SmtOpSymbol::make_model(VariableGenerator *vg, SymbolGetter *sg)
{
    SmtModel *model = vg->getModel();
    std::vector<SmtTerm *> *keys = model->keys();

    ConnectedComponent *satAssnSetK = sg->getKind("SatAssignmentSet");
    ConnectedComponent *smtCheckResK = sg->getKind("SmtCheckResult");
    ConnectedComponent *satAssnK = sg->getKind("SatAssignment");

    Vector<ConnectedComponent *> dom;
    Symbol *emptySatAssnSet = sg->getSymbol("empty", dom, satAssnSetK);

    dom.push_back(satAssnSetK);
    Symbol *satAssnSet = sg->getSymbol("{_}", dom, smtCheckResK);

    dom.push_back(satAssnSetK);
    Symbol *concatSatAssnSet = sg->getSymbol("_,_", dom, satAssnSetK);

    Converter *conv = vg->getConverter();
    DagNode *result = emptySatAssnSet->makeDagNode();
    for (auto k : *keys)
    {
        dom.clear();
        DagNode *kd = conv->term2dag(k);
        DagNode *kvd = conv->term2dag(model->get(k));

        if (kd == nullptr || kvd == nullptr)
        {
            // cout << k << endl;
            // cout << model->get(k) << endl;
            continue;
        }

        Symbol *assn = nullptr;
        ConnectedComponent *kdS = kd->getSort()->component();

        Sort *ik = sg->getSort("Integer");
        Sort *rk = sg->getSort("Real");
        Sort *bk = sg->getSort("Boolean");

        if (ik && kdS == ik->component())
        {
            dom.push_back(ik->component());
            dom.push_back(ik->component());

            assn = sg->getSymbol("_|->_", dom, satAssnK);
        }

        if (rk && kdS == rk->component())
        {
            dom.push_back(rk->component());
            dom.push_back(rk->component());

            assn = sg->getSymbol("_|->_", dom, satAssnK);
        }

        if (bk && kdS == bk->component())
        {
            dom.push_back(bk->component());
            dom.push_back(bk->component());

            assn = sg->getSymbol("_|->_", dom, satAssnK);
        }

        if (assn == nullptr)
            continue;

        Vector<DagNode *> args(2);
        args[0] = kd;
        args[1] = kvd;

        Vector<DagNode *> r(2);
        r[0] = result;
        r[1] = assn->makeDagNode(args);

        result = concatSatAssnSet->makeDagNode(r);
    }
    Vector<DagNode *> dag(1);
    dag[0] = result;

    return satAssnSet->makeDagNode(dag);
}