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
			bool genAssn;
			if (!metaLevel->downBool(subject->getArgument(3), genAssn))
			{
				return false;
			}
			m->protect();
			term = term->normalize(false);
			DagNode *d = term->term2Dag();

			const SMT_Info &smtInfo = m->getSMT_Info();
			const char* logic = downLogic(subject->getArgument(2));
			//   VariableGenerator vg(smtInfo);
			//   VariableGenerator::Result result = vg.checkDag(d);
			// VariableGenerator *vg = smtManagerFactory->create(smtInfo);
			VariableGenerator* vg = new VariableGenerator(smtInfo, dynamic_cast<MetaLevelSmtOpSymbol*>(const_cast<MetaLevelSmtOpSymbol*>(this)));
			vg->getConverter()->prepareFor(m);
			vg->getConnector()->set_logic(logic);

			VariableGenerator::Result result = vg->assertDag(d);
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
				if (genAssn && result == VariableGenerator::SAT){
					// This is Hack:
					// we use symbol in the downed module, while we use subject's module symbol to create new expressions.
					// We should upDagNode with different modules.
					// MixfixModule* module = safeCast(MixfixModule*, subject->symbol()->getModule());
					r = make_model(vg, m);
				} else if (result == VariableGenerator::SAT_UNKNOWN){
					r = this->unknownResultSymbol->makeDagNode();
				} else {
					r = metaLevel->upBool(result == VariableGenerator::SAT);
				}

				term->deepSelfDestruct();
				(void)m->unprotect();
				delete vg;

				return context.builtInReplace(subject, r);
			}
			}

			term->deepSelfDestruct();
			(void)m->unprotect();
			delete vg;
		}
	}
	return false;
}

DagNode*  MetaLevelSmtOpSymbol::make_model(VariableGenerator* vg, MixfixModule* m){
	SmtModel* model = vg->getModel();
	std::vector<SmtTerm*>* keys = model->keys();

	Converter* conv = vg->getConverter();
	DagNode* result = emptySatAssnSetSymbol->makeDagNode();
  	PointerMap qidMap;
	PointerMap dagMap;
	MetaLevel* metaLevel = getMetaLevel();

	for (auto k : *keys){
		DagNode* kd = conv->term2dag(k); 
		DagNode* kvd = conv->term2dag(model->get(k));

		if (kd == nullptr || kvd == nullptr){
			// cout << k << endl;
			// cout << model->get(k) << endl;
			continue;
		}

		Vector < DagNode * > satAssn(2);
		satAssn[0] = metaLevel->upDagNode(kd, m, qidMap, dagMap);
		satAssn[1] = metaLevel->upDagNode(kvd, m, qidMap, dagMap);
		
		Vector < DagNode * > r(2);
		r[0] = result;
		r[1] = satAssnSymbol->makeDagNode(satAssn);

		result = concatSatAssnSetSymbol->makeDagNode(r);
	}
	Vector < DagNode* > dag(1);
	dag[0] = result;

	return satAssnSetSymbol->makeDagNode(dag);
}

void printDagNodeGy(DagNode* dagNode){
	cout << dagNode << endl;
}