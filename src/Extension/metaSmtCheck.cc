#include "SMT_Info.hh"
// #include "variableGenerator.hh"
#include "smtManager.hh"
// #include "smtManagerFactory.hh"
#include "extGlobal.hh"

bool MetaLevelSmtOpSymbol::metaSmtCheck(FreeDagNode *subject, RewritingContext &context)
{
	if (VisibleModule *m = metaLevel->downModule(subject->getArgument(0)))
	{
		if (Term *term = metaLevel->downTerm(subject->getArgument(1), m))
		{
			SymbolGetter sg;
			sg.setModule(m);

			SymbolGetter opSg;
			opSg.setModule(getCurrentModule());

			bool genAssn;
			if (!metaLevel->downBool(subject->getArgument(3), genAssn))
			{
				return false;
			}
			m->protect();
			term = term->normalize(false);
			DagNode *d = term->term2Dag();

			const SMT_Info &smtInfo = m->getSMT_Info();
			const char *logic = downLogic(subject->getArgument(2));

			VariableGenerator vg(smtInfo, false);
			vg.getConverter()->prepareFor(m);

			if (logic)
				vg.getConnector()->set_logic(logic);

			VariableGenerator::Result result = vg.assertDag(d);
			switch (result)
			{
			case VariableGenerator::BAD_DAG:
			{
				IssueAdvisory("term " << QUOTE(term) << " is not a valid SMT Boolean expression.");
				break;
			}
			case VariableGenerator::SAT_UNKNOWN:
			case VariableGenerator::UNSAT:
			case VariableGenerator::SAT:
			{
				DagNode *r;
				if (genAssn && result == VariableGenerator::SAT)
				{
					// This is Hack:
					// we use symbol in the downed module, while we use subject's module symbol to create new expressions.
					// We should upDagNode with different modules.
					// MixfixModule* module = safeCast(MixfixModule*, subject->symbol()->getModule());
					r = make_model(&vg, m, &opSg);
				}
				else if (result == VariableGenerator::SAT_UNKNOWN)
				{
					r = this->unknownResultSymbol->makeDagNode();
				}
				else
				{
					r = metaLevel->upBool(result == VariableGenerator::SAT);
				}

				term->deepSelfDestruct();
				(void)m->unprotect();

				return context.builtInReplace(subject, r);
			}
			}

			term->deepSelfDestruct();
			(void)m->unprotect();
		}
	}
	return false;
}

DagNode *MetaLevelSmtOpSymbol::make_model(VariableGenerator *vg, MixfixModule *m, SymbolGetter *sg)
{
	SmtModel *model = vg->getModel();
	std::vector<SmtTerm *> *keys = model->keys();

	ConnectedComponent *satAssnSetK = sg->getKind("SatAssignmentSet");
	ConnectedComponent *smtCheckResK = sg->getKind("SmtCheckResult");
	ConnectedComponent *satAssnK = sg->getKind("SatAssignment");
	ConnectedComponent *termK = sg->getKind("Term");

	Vector<ConnectedComponent *> dom;
	Symbol *emptySatAssnSet = sg->getSymbol("empty", dom, satAssnSetK);

	dom.push_back(satAssnSetK);
	Symbol *satAssnSet = sg->getSymbol("{_}", dom, smtCheckResK);

	dom.push_back(satAssnSetK);
	Symbol *concatSatAssnSet = sg->getSymbol("_,_", dom, satAssnSetK);

	dom.clear();
	dom.push_back(termK);
	dom.push_back(termK);

	Symbol *satAssn = sg->getSymbol("_|->_", dom, satAssnK);

	Converter *conv = vg->getConverter();

	DagNode *result = emptySatAssnSet->makeDagNode();

	PointerMap qidMap;
	PointerMap dagMap;
	MetaLevel *metaLevel = getMetaLevel();

	for (auto k : *keys)
	{
		DagNode *kd = conv->term2dag(k);
		DagNode *kvd = conv->term2dag(model->get(k));

		if (kd == nullptr || kvd == nullptr)
		{
			// cout << k << endl;
			// cout << model->get(k) << endl;
			continue;
		}

		Vector<DagNode *> args(2);
		args[0] = metaLevel->upDagNode(kd, m, qidMap, dagMap);
		args[1] = metaLevel->upDagNode(kvd, m, qidMap, dagMap);

		Vector<DagNode *> r(2);
		r[0] = result;
		r[1] = satAssn->makeDagNode(args);

		result = concatSatAssnSet->makeDagNode(r);
	}
	Vector<DagNode *> dag(1);
	dag[0] = result;

	return satAssnSet->makeDagNode(dag);
}