//
//	Implementation for class SmtStateTransitionGraph.
//

//	utility stuff
#include "macros.hh"
#include "vector.hh"

//	forward declarations
#include "interface.hh"
#include "core.hh"
#include "higher.hh"
#include "mixfix.hh"

#include "strategyLanguage.hh"
#include "interpreter.hh"
#include "visibleModule.hh"

//	interface class definitions
#include "symbol.hh"
#include "dagNode.hh"
#include "variableDagNode.hh"
#include "rawDagArgumentIterator.hh"
#include "term.hh"
#include "lhsAutomaton.hh"
#include "subproblem.hh"
#include "rewriteSmtSearchState.hh"
#include "variableInfo.hh"
#include "smtStateTransitionGraph.hh"
#include "freshVariableGenerator.hh"
#include "token.hh"
#include "smtManager.hh"

SmtStateTransitionGraph::SmtStateTransitionGraph(RewritingContext *initial,
												 const SMT_Info &smtInfo, SMT_EngineWrapper *engine,
												 FreshVariableGenerator *freshVariableGenerator,
												 bool fold, bool merge,
												 const mpz_class &avoidVariableNumber)
	: initial(initial), smtInfo(smtInfo), engine(engine), fold(fold), merge(merge),
	  freshVariableGenerator(freshVariableGenerator), stateCollection(fold)
{
	if (fold)
		Verbose("folding option is on");
	else
		Verbose("folding option is off");

	initial->reduce();

	counter = 0;
	initState = new State(counter, NONE);
	initState->dag = initial->root();
	initState->avoidVariableNumber = avoidVariableNumber;
	initState->depth = 0;

	map2seen.insert(Map2Seen::value_type(make_tuple(counter, 0), seen.size()));
	seen.append(initState);

	int idx;
	stateCollection.insertState(counter, initial->root(), NONE, &idx);
	counter++;

	nextTime = 0.0;
	rewriteTime = 0.0;

	VariableGenerator *vg = dynamic_cast<VariableGenerator *>(engine);
	connector = vg->getConnector();
	connector2 = vg->getConnector2();
	conv = vg->getConverter();
}

SmtStateTransitionGraph::~SmtStateTransitionGraph()
{
	int nrStates = seen.length();
	for (int i = 0; i < nrStates; i++)
	{
		// this is double free?
		delete seen[i]->rewriteState;
		delete seen[i];
	}

	for (auto &it : consTermSeen)
	{
		for (auto &ct : it.second)
		{
			delete ct;
		}
	}
	delete initial;
}

int SmtStateTransitionGraph::getNextState(int stateNr, int index)
{
	State *n = seen[stateNr];
	int nrNextStates = n->nextStates.length();
	if (index < nrNextStates)
		return n->nextStates[index];
	if (n->fullyExplored)
		return NONE;

	clock_t start = clock();
	if (n->rewriteState == 0)
	{
		DagNode *canonicalStateDag = consTermSeen[n->hashConsIndex][n->constTermIndex]->dag;
		//   stateCollection.getState(n->hashConsIndex, canonicalStateDag);

		// ConstrainedTerm* constTerm = consTermSeen[n->hashConsIndex][];
		// DagNode* canonicalStateDag = constTerm->dag;

		Verbose("  make a new rewrite state with " << canonicalStateDag);

		RewritingContext *newContext = initial->makeSubcontext(canonicalStateDag);
		n->rewriteState = new RewriteSmtSearchState(newContext, engine, freshVariableGenerator,
													n->avoidVariableNumber,
													NONE,
													RewriteSmtSearchState::GC_CONTEXT |
														RewriteSmtSearchState::SET_UNREWRITABLE |
														PositionState::SET_UNSTACKABLE,
													0,
													UNBOUNDED);
	}

	// DagNode* dndp;
	//   stateCollection.getState(n->hashConsIndex, dndp);

	// ConstrainedTerm *constrained = consTermSeen[n->hashConsIndex][n->constTermIndex];
	// PyObject *objectsRepresentation = PyObject_Repr(constrained->constraint);
	// const char *ss = PyUnicode_AsUTF8(objectsRepresentation);

	// const void *address = static_cast<const void *>(constrained->dag);
	// std::stringstream strs;
	// strs << address;
	// string adrId = strs.str();
	// Verbose("  try to find next state of " << n->hashConsIndex << " and const # " << n->constTermIndex);
	// Verbose("  dag : ");
	// Verbose("  " << constrained->dag);
	// Verbose("  address : ");
	// Verbose("  " << adrId.c_str());
	// Verbose("  const : ");
	// Verbose("  " << ss);

	RewriteSmtSearchState *rewriteState = n->rewriteState;
	RewritingContext *context = rewriteState->getContext();
	Verbose("    actual dag : " << context->root());

	while (nrNextStates <= index)
	{
		Verbose("    nrNextStates " << nrNextStates << ", index : " << index);
		Verbose("  - searching depth : " << n->depth + 1);
		clock_t r_s = clock();
		bool success = rewriteState->findNextRewrite();
		clock_t e_s = clock();

		rewriteTime += (double)(e_s - r_s);
		rewriteState->transferCountTo(*initial);

		Verbose("    found next rewrite? " << success);

		if (success)
		{
			Rule *rule = rewriteState->getRule();
			bool trace = RewritingContext::getTraceStatus();
			if (trace)
			{
				context->tracePreRuleRewrite(rewriteState->getDagNode(), rule);
				if (context->traceAbort())
					return NONE;
			}

			// get replacement node
			DagNode *replacement = rewriteState->getReplacement();
			RewriteSmtSearchState::DagPair r = rewriteState->rebuildDag(replacement);
			RewritingContext *c = context->makeSubcontext(r.first);
			initial->incrementRlCount();
			if (trace)
			{
				c->tracePostRuleRewrite(r.second);
				if (c->traceAbort())
				{
					delete c;
					return NONE;
				}
			}
			c->reduce();
			if (c->traceAbort())
			{
				delete c;
				return NONE;
			}
			initial->addInCount(*c);
			delete c;

			//  assume the dag has two arugments ...

			RawDagArgumentIterator *arg = r.first->arguments();
			Vector<DagNode *> dg;
			int nrChild = 0;
			while (arg->valid())
			{
				nrChild++;
				dg.append(arg->argument());
				arg->next();
			}

			if (nrChild != 2)
			{
				IssueWarning("failed to apply one-step symbolic rewrite (error term : " << r.first << ")");
			}

			DagNode *c1 = dg[0];
			DagNode *c2 = dg[1];
			// c1->mark();

			Verbose("first argument is " << c1 << " and second is " << c2);

			// accumulated constraints
			SmtTerm acc = consTermSeen[n->hashConsIndex][n->constTermIndex]->constraint;
			// we have pair of terms (norm, prev)
			SmtTerm cur = conv->dag2term(c2);

			// Py_XINCREF(acc);

			// PyObject *result = PyObject_CallMethodObjArgs(connector, check_sat, acc, cur, NULL);
			connector->push();
			SmtTermVector ll = std::make_shared<std::vector<SmtTerm>>();
			ll->push_back(acc);
			ll->push_back(cur);

			if (connector->check_sat(ll) != sat)
			{
				Verbose("constraint is unsatisfiable ... continue");
				connector->pop();
				continue;
			}
			connector->pop();

			int nextState;
			int index2;
			bool needMerge = !fold;

			ConstrainedTerm *newConsTerm = new ConstrainedTerm(c1, connector->add_const(acc, cur));

			DagNode *reprDag;
			// folding case ...
			if (stateCollection.insertState(counter, c1, NONE, &index2))
			{
				reprDag = c1;
				if (!merge)
				{
					nextState = seen.size();

					State *newState = new State(counter, stateNr);
					newState->avoidVariableNumber = n->rewriteState->getMaxVariableNumber();
					newState->dag = c1;
					newState->depth = n->depth + 1;

					newState->constTermIndex = 0;
					consTermSeen.insert(ConstrainedTermMap::value_type(counter, Vector<ConstrainedTerm *>()));
					consTermSeen[counter].append(newConsTerm);
					map2seen.insert(Map2Seen::value_type(make_tuple(counter, 0), seen.size()));
					seen.append(newState);

					counter++;
					needMerge = false;
				}
				else
				{
					needMerge = true;
				}
			}
			else
			{
				stateCollection.getState(index2, reprDag);

				auto it = consTermSeen.find(index2);
				bool exists = false;
				if (it != consTermSeen.end())
				{
					int cc = 0;
					for (auto &constTerm : it->second)
					{
						Verbose("  check folding from " << c1 << " to " << constTerm->dag);
						// check the conjunt dag is subsumed by an older one
						bool isMatch = constTerm->findMatching(c1, conv, connector2);

						if (!isMatch)
						{
							IssueWarning("subsumption is wrong (" << constTerm->dag << " should be renaming equivalent with " << c1 << ")");
						}

						connector2->push();
						bool subsumedResult = connector2->subsume(constTerm->subst, constTerm->constraint, acc, cur);
						connector2->pop();

						if (subsumedResult)
						{
							Verbose("constraints subsumed by another");
							nextState = map2seen[make_tuple(index2, cc)];
							exists = true;
							needMerge = false;
							break;
						}
						cc++;
					}
				}

				if (!exists)
				{
					Verbose("constraints not subsumed by any others");
					if (!merge)
					{
						nextState = seen.size();
						State *newState = new State(index2, stateNr);
						newState->avoidVariableNumber = n->rewriteState->getMaxVariableNumber();
						newState->constTermIndex = consTermSeen[index2].size();
						newState->dag = reprDag;
						newState->depth = n->depth + 1;

						consTermSeen[index2].append(newConsTerm);
						map2seen.insert(Map2Seen::value_type(make_tuple(index2, newState->constTermIndex), seen.size()));
						seen.append(newState);
						needMerge = false;
					}
					else
					{
						needMerge = true;
					}
				}
			}

			if (merge && needMerge)
			{
				// bool foundMergable = false;
				// // make a dummy constrained term for matching
				// ConstrainedTerm *t = new ConstrainedTerm(c1, nullptr);

				// // find mergable state
				// PyObject *newConstTerm = nullptr;
				// PyObject *prevPyDag = nullptr;
				// PyObject *curPyDag = nullptr;

				// int foundStateIndex = NONE;
				// int foundConstIndex = 0;
				// for (State *constTermState : seen)
				// {
				// 	foundStateIndex++;

				// 	// only apply for the states with the same depth
				// 	if (constTermState->depth != n->depth + 1)
				// 	{
				// 		continue;
				// 	}

				// 	int ctIdx = 0;
				// 	for (auto constTerm : consTermSeen[constTermState->hashConsIndex])
				// 	{
				// 		bool isMatch = t->findMatching(constTerm->dag, conv, connector);
				// 		Verbose("  check matching for merging from ");
				// 		Verbose("  " << t->dag);
				// 		Verbose("  " << constTerm->dag);
				// 		Verbose("  result : " << isMatch);

				// 		if (isMatch)
				// 		{
				// 			Verbose("inside merge: " << c1 << " matches " << constTerm->dag);

				// 			prevPyDag = dag2maudeTerm(constTerm->dag);
				// 			curPyDag = dag2maudeTerm(t->dag);

				// 			newConstTerm = PyObject_CallMethodObjArgs(connector, mergeF, t->subst, prevPyDag, constTerm->constraint, curPyDag, acc, cur, NULL);
				// 			if (newConstTerm == nullptr)
				// 			{
				// 				IssueWarning("failed to apply merging");
				// 			}

				// 			foundMergable = true;
				// 			foundConstIndex = ctIdx;
				// 			// cout << " found " << endl;
				// 			break;
				// 		}
				// 		ctIdx++;
				// 	}

				// 	// find mergable state
				// 	if (foundMergable)
				// 		break;
				// }

				// if (foundMergable)
				// {
				// 	State *foundState = seen[foundStateIndex];

				// 	// 1) the constraint after merging must be given
				// 	// 2) assertion, mergable states must not be explored yet
				// 	// 3) its depth is the same as the merged state
				// 	if (newConstTerm == nullptr || foundState->rewriteState != 0 ||
				// 		foundState->depth != n->depth + 1)
				// 	{
				// 		IssueWarning("invalid state is used to merging");
				// 	}

				// 	// get term
				// 	PyObject *term = PyTuple_GetItem(newConstTerm, 0);
				// 	PyObject *newConst = PyTuple_GetItem(newConstTerm, 1);

				// 	if (term == nullptr || newConst == nullptr)
				// 	{
				// 		IssueWarning("failed to retreive a constrained term");
				// 	}

				// 	// get dag
				// 	DagNode *mergableDag = maudeTerm2dag(term);

				// 	// need to compute sort
				// 	// otherwise, matching would fail
				// 	RewritingContext *sort_context = initial->makeSubcontext(mergableDag);
				// 	mergableDag->computeTrueSort(*sort_context);
				// 	delete sort_context;

				// 	ConstrainedTerm *constTerm = consTermSeen[foundState->hashConsIndex][foundConstIndex];
				// 	delete constTerm;

				// 	consTermSeen[foundState->hashConsIndex][foundConstIndex] = new ConstrainedTerm(mergableDag, newConst);

				// 	nextState = foundStateIndex;
				// 	delete t;
				// }
				// else
				// {
				// 	nextState = seen.size();

				// 	State *newState = new State(counter, stateNr);
				// 	newState->avoidVariableNumber = n->rewriteState->getMaxVariableNumber();
				// 	newState->dag = reprDag;
				// 	newState->depth = n->depth + 1;

				// 	t->constraint = PyObject_CallMethodObjArgs(connector, add_const, acc, cur, NULL);
				// 	if (t->constraint == nullptr)
				// 	{
				// 		IssueWarning("failed to make a constraint");
				// 	}

				// 	newState->constTermIndex = 0;
				// 	consTermSeen.insert(ConstrainedTermMap::value_type(counter, Vector<ConstrainedTerm *>()));
				// 	consTermSeen[counter].append(t);
				// 	map2seen.insert(Map2Seen::value_type(make_tuple(counter, 0), seen.size()));
				// 	seen.append(newState);

				// 	counter++;
				// }
			}

			n->nextStates.append(nextState);
			n->fwdArcs[nextState].insert(rule);
			++nrNextStates;
			// cout << "next reeady" << endl;
			//
			//	If we didn't do any equational rewriting we will not have had a chance to
			//	collect garbage.
			//
			MemoryCell::okToCollectGarbage();

			// cout << "after garbage" << endl;
		}
		else
		{
			Verbose("matching failed...");
			delete rewriteState;
			n->fullyExplored = true;
			n->rewriteState = 0;

			clock_t end = clock();
			nextTime += (double)(end - start);
			return NONE;
		}
	}
	clock_t end = clock();
	nextTime += (double)(end - start);
	return n->nextStates[index];
}

void SmtStateTransitionGraph::printStateConst(int depth)
{
	// for (auto state : seen)
	// {
	// 	if (state->depth == depth)
	// 	{
	// 		cout << "    state : " << state->dag << endl;
	// 		int cc = 0;
	// 		for (auto it : consTermSeen[state->hashConsIndex])
	// 		{
	// 			PyObject *str = PyObject_Repr(it->constraint);
	// 			const char *str_c = PyUnicode_AsUTF8(str);
	// 			Py_XDECREF(str);

	// 			cout << "      dag   [" << cc + 1 << "]" << it->dag << endl;
	// 			cout << "      const [" << cc + 1 << "]" << str_c << endl;
	// 			cout << endl;

	// 			cc++;
	// 		}
	// 	}
	// }
}

SmtStateTransitionGraph::ConstrainedTerm::ConstrainedTerm(DagNode *dag, SmtTerm constraint)
	: dag(dag), constraint(constraint)
{
	Term *t = dag->symbol()->termify(dag);
	//
	//	Even thoug t should be in normal form we need to set hash values.
	//
	t = t->normalize(true);
	// VariableInfo variableInfo;
	t->indexVariables(variableInfo);
	t->symbol()->fillInSortInfo(t);
	t->analyseCollapses();

	NatSet boundUniquely;
	bool subproblemLikely;

	t->determineContextVariables();
	t->insertAbstractionVariables(variableInfo);

	matchingAutomaton = t->compileLhs(false, variableInfo, boundUniquely, subproblemLikely);
	term = t;
	nrMatchingVariables = variableInfo.getNrProtectedVariables();

	subst = nullptr;
}

SmtStateTransitionGraph::ConstrainedTerm::~ConstrainedTerm()
{
	delete matchingAutomaton;
	if (term)
		term->deepSelfDestruct();
}

bool SmtStateTransitionGraph::ConstrainedTerm::findMatching(DagNode *other, Converter converter, Connector connector)
{
	MemoryCell::okToCollectGarbage(); // otherwise we have huge accumulation of junk from matching
	// DO NOT: this will cause memory corruption
	// if (subst){
	// 	Py_DECREF(subst);
	// }
	// cout << "dag : " << dag << " checking matching with " << other << endl;

	int nrSlotsToAllocate = nrMatchingVariables;
	if (nrSlotsToAllocate == 0)
		nrSlotsToAllocate = 1; // substitutions subject to clear() must always have at least one slot

	RewritingContext matcher(nrSlotsToAllocate);
	matcher.clear(nrMatchingVariables);
	Subproblem *subproblem = 0;

	bool result = matchingAutomaton->match(other, matcher, subproblem) &&
				  (subproblem == 0 || subproblem->solve(true, matcher));
	delete subproblem;

	// delete old subst, if any exists
	// if (subst) {
	// 	subst = nullptr;
	// }

	if (result)
	{
		int maxSize = matcher.nrFragileBindings();
		std::map<DagNode *, DagNode *> subst_dict;
		for (int i = 0; i < maxSize; i++)
		{
			Term *v_term = variableInfo.index2Variable(i);

			DagNode *left = v_term->term2Dag();
			DagNode *right = matcher.value(i);

			subst_dict.insert(std::pair<DagNode *, DagNode *>(left, right));
		}
		subst = connector->mk_subst(subst_dict);
	}
	return result;
}
