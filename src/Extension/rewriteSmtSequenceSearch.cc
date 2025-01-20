//
//	Implementation for class RewriteSmtSequenceSearch.
//

//	utility stuff
#include "macros.hh"
#include "vector.hh"

//	forward declarations
#include "interface.hh"
#include "core.hh"
#include "higher.hh"
#include "mixfix.hh"

//	interface class definitions
#include "symbol.hh"
#include "dagNode.hh"
#include "rawDagArgumentIterator.hh"

//	core class definitions
#include "rewritingContext.hh"
#include "pattern.hh"
#include "rewriteSmtSearchState.hh"
#include "rewriteSmtSequenceSearch.hh"

#include "../StrategyLanguage/strategyLanguage.hh"
#include "../Mixfix/mixfixModule.hh"
#include "freshVariableSource.hh"
#include "token.hh"
#include "equalityConditionFragment.hh" // for printing purpose ...
#include <ctime>

RewriteSmtSequenceSearch::RewriteSmtSequenceSearch(RewritingContext *initial,
                                                   SearchType searchType,
                                                   Pattern *goal, Pattern* smtGoal,
                                                   const SMT_Info &smtInfo,
                                                   SMT_EngineWrapper *engine,
                                                   FreshVariableGenerator *freshVariableGenerator,
                                                   bool fold, bool merge,
                                                   int maxDepth,
                                                   const mpz_class &avoidVariableNumber)
    : SmtStateTransitionGraph(initial, smtInfo, engine, freshVariableGenerator, fold, merge, avoidVariableNumber),
      goal(goal), smtGoal(smtGoal),
      maxDepth((searchType == ONE_STEP) ? 1 : maxDepth)
{
  initState->constTermIndex = consTermSeen[initState->hashConsIndex].size();
  // DagNode *initConst = makeConstraintFromCondition(smtGoal->getCondition());

	DagNode* trueDag = smtInfo.getTrueSymbol()->makeDagNode();
	trueDag->computeTrueSort(*initial);

  // SmtTerm *initRi = convDag2Term(trueDag);

  smtGoalConst = smtGoal->getLhs()->term2Dag();
  SmtTerm *initRi = convDag2Term(smtGoalConst);
  // SmtTerm *initRi = convDag2Term(initConst);

  // PyObject *next = PyObject_CallMethodObjArgs(connector, add_const, Py_None, initRi, NULL);
  SmtTerm* next = connector->add_const(nullptr, initRi);
  if (next == nullptr)
  {
    IssueWarning("failed to translate an initial SMT constraint to a solver term");
  }

  // Py_XINCREF(next);
  ConstrainedTerm *t = new ConstrainedTerm(initial->root(), next);

  consTermSeen.insert(ConstrainedTermMap::value_type(initState->hashConsIndex, Vector<ConstrainedTerm *>()));
  consTermSeen[initState->hashConsIndex].append(t);

  findSMT_Variables();

  newVariableNumber = 0;

  // finalConstraint = 0;
  matchState = 0;
  explore = -1;
  exploreDepth = -1;
  firstDeeperNodeNr = 0;
  needToTryInitialState = (searchType == ANY_STEPS);
  reachingInitialStateOK = (searchType == AT_LEAST_ONE_STEP || searchType == ONE_STEP);
  normalFormNeeded = (searchType == NORMAL_FORM);
  nextArc = NONE;

  time = 0.0;
  // Verbose("RewriteSmtSeqSearch : " << this << " [parent : " << static_cast<SmtStateTransitionGraph*>(this) << "]" 
  // << " [rootContainer : " << static_cast<RootContainer*>(this) << "]" << " [folder : " << &stateCollection << "] (alloc)");
}

RewriteSmtSequenceSearch::~RewriteSmtSequenceSearch()
{
  delete matchState;
  delete goal;
  delete smtGoal;
  delete engine;
  // Verbose("RewriteSmtSeqSearch : " << this << "[parent : " << static_cast<SmtStateTransitionGraph*>(this) << "] " 
  // << " [rootContainer : " << static_cast<RootContainer*>(this) << "] (free)");
}

void RewriteSmtSequenceSearch::markReachableNodes()
{
  // cout << "marking is called" << endl;
  //
  //	Protect dagnode versions of any SMT variables in the pattern.
  //
  for (auto &i : smtVarDags)
    i.second->mark();
  //
  //	Constraints aren't otherwise protected once the search object
  //	they were passed to is deleted.
  //
  for (State *s : seen)
  {
    s->dag->mark();
    // cout << "marking " << s->dag << endl;
  }

  for (auto it = consTermSeen.begin(); it != consTermSeen.end(); it++)
  {
    for (ConstrainedTerm *c : it->second)
    {
      c->dag->mark();
      // for (auto m : c->mapping){
      //   m.first->mark();
      //   m.second->mark();
      // }
    }
  }
  //
  //	Need to protect any final constraint we made.
  //
  // if (finalConstraint != 0)
  //   finalConstraint->mark();
  if (smtGoalConst)
    smtGoalConst->mark();
}

bool RewriteSmtSequenceSearch::findNextMatch()
{
  if (matchState != 0)
    goto tryMatch; // non-startup case

  for (;;)
  {
    {
    clock_t start = clock();
    stateNr = findNextInterestingState();
    clock_t end = clock();

    time += (double)(end - start);
    }
    if (stateNr == NONE)
      break;

    { // To avoid "jump_to_label error", we wrap this block
      DagNode* stateDag = getStateDag(stateNr);

      Verbose("\n");
      Verbose("  goal pattern : ");
      Verbose("    " << goal->getLhs());
      Verbose("  checking pattern matching with the term : ");
      Verbose("    " << stateDag);
      Verbose("  term's internal state # " << stateNr);

      matchState = new MatchSearchState(getContext()->makeSubcontext(stateDag),
                                        goal,
                                        MatchSearchState::GC_CONTEXT);
    }

  tryMatch:
    bool foundMatch = matchState->findNextMatch();

    Verbose("found match : " << foundMatch);
    matchState->transferCountTo(*(getContext()));
    if (foundMatch && checkMatchConstraint(stateNr))
    {
      Verbose("goal sat with final constraint");
      // cout << "time took : " << time / CLOCKS_PER_SEC << endl;
      // cout << "get next state time : " << nextTime / CLOCKS_PER_SEC << endl;
      // cout << "match rewrite time : " << rewriteTime / CLOCKS_PER_SEC << endl;
      // cout << "else time : " << elseTime / CLOCKS_PER_SEC << endl;

      return true;
    }

    delete matchState;
  }

  matchState = 0;
  return false;
}

int RewriteSmtSequenceSearch::findNextInterestingState()
{
  if (needToTryInitialState)
  {
    //
    //	Special case: return the initial state.
    //
    needToTryInitialState = false; // don't do this again
    return 0;
  }

  if (nextArc != NONE)
    goto exploreArcs;

  for (;;)
  {
    //
    //	Get next state to explore.
    //
    ++explore;
    if (explore == getNrStates())
      break;
    if (explore == firstDeeperNodeNr)
    {
      ++exploreDepth;
      if (normalFormNeeded)
      {
        if (maxDepth > 0 && exploreDepth > maxDepth)
          break;
      }
      else
      {
        if (exploreDepth == maxDepth)
          break;
      }
      firstDeeperNodeNr = getNrStates();
    }
    nextArc = 0;

  exploreArcs:
    int nrStates = getNrStates();
    int nextStateNr;
    while ((nextStateNr = getNextState(explore, nextArc)) != NONE)
    {
      ++nextArc;
      if (normalFormNeeded)
      {
        if (exploreDepth == maxDepth)
          break; // no point looking for further arcs
      }
      else
      {
        if (nextStateNr == nrStates)
        { // new state reached
          Verbose("add a new state " << nextStateNr);
          return nextStateNr;
        }
        if (nextStateNr == 0 && reachingInitialStateOK)
        {
          //
          //	We have arrived back at our initial state, but because
          //	we didn't try matching the initial state, we do it now.
          //
          reachingInitialStateOK = false; // don't do this again
          return 0;
        }
      }
    }
    if (getContext()->traceAbort())
      return NONE;
    if (normalFormNeeded && nextArc == 0)
    {
      nextArc = NONE;
      return explore;
    }
  }
  return NONE;
}

Rule *
RewriteSmtSequenceSearch::getStateRule(int stateNr) const
{
  const ArcMap &fwdArcs = getStateFwdArcs(getStateParent(stateNr));
  return *(fwdArcs.find(stateNr)->second.begin());
}

void RewriteSmtSequenceSearch::findSMT_Variables()
{
  //
  //	Find any SMT variables in the pattern, make dagnode versions and record their indices.
  //
  int nrVariables = goal->getNrRealVariables();
  for (int i = 0; i < nrVariables; ++i)
  {
    VariableTerm *v = safeCast(VariableTerm *, goal->index2Variable(i));
    VariableSymbol *vs = safeCast(VariableSymbol *, v->symbol());
    SMT_Info::SMT_Type type = smtInfo.getType(vs->getSort());
    if (type != SMT_Info::NOT_SMT)
    {
      smtVarIndices.insert(i);
      smtVarDags[i] = v->dagify2();
      // cout << "found " << smtVarDags[i] << endl;
    }
  }
  Verbose("found " << smtVarDags.size() << " SMT variables");
}

bool RewriteSmtSequenceSearch::checkMatchConstraint(int stateNr)
{
  //
  //	We have a matching substitution, but some of the bound variables may be SMT
  //	in which case they may be mentioned in the existing condition and we
  //	need to check that equality implied by the binding is satisfiable.
  //
  Vector<DagNode *> args(2);
  const Substitution *substitution = matchState->getContext();
  // DagNode *matchConstraint = smtGoalConst;
  DagNode* matchConstraint = nullptr;
  // for (auto &i : smtVarDags)
  // {
  //   Verbose("smtVarDags " << i.first << " : " << i.second);
  // }

  for (auto &i : smtVarDags)
  {
    DagNode *lhs = i.second;
    DagNode *rhs = substitution->value(i.first);
    //
    //	Make equality constraint.
    //
    Vector<DagNode *> args(2);
    args[0] = lhs;
    args[1] = rhs;
    DagNode *equalityConstraint = smtInfo.getEqualityOperator(lhs, rhs)->makeDagNode(args);
    //
    //	Conjunct it in if needed.
    //
    if (matchConstraint == 0)
    {
      matchConstraint = equalityConstraint;
    }
    else
    {
      args[0] = matchConstraint;
      args[1] = equalityConstraint;
      matchConstraint = smtInfo.getConjunctionOperator()->makeDagNode(args);
    }
  }

  ConstrainedTerm *constrained = consTermSeen[seen[stateNr]->hashConsIndex][seen[stateNr]->constTermIndex];
  std::vector<SmtTerm*> ll;
  ll.push_back(constrained->constraint);
  SmtTerm* matchTerm = 0;

  if (matchConstraint)
  {
    Verbose("matchConstraint: " << matchConstraint);
    matchTerm = convDag2Term(matchConstraint);
    ll.push_back(matchTerm);
  } 

  connector->push();
  if (!connector->check_sat(ll))
  {
    connector->pop();
    return false;
  }
  else 
  {
    // get a model
    constrained->model = connector->get_model();
    connector->pop();
    // update acc const if matching term exists
    if (matchTerm){
      constrained->constraint = connector->add_const(constrained->constraint, matchTerm);
    }
  }
  return true;
}

DagNode *
RewriteSmtSequenceSearch::makeConstraintFromCondition(const Vector<ConditionFragment *> &condition)
{
  Vector<DagNode *> args(2);
  DagNode *constraint = 0;

  for (ConditionFragment *cf : condition)
  {
    //
    //	Check to see that condition fragment is of the form t1 = t2.
    //
    EqualityConditionFragment *fragment = dynamic_cast<EqualityConditionFragment *>(cf);
    if (fragment == 0)
    {
      IssueWarning("goal... : condition fragment " << cf << " not supported for searching modulo SMT.");
      continue;
    }
    //
    //	Dagify and optimize out equal case.
    //
    fragment->normalize(false);
    DagNode *lhs = fragment->getLhs()->term2Dag();
    DagNode *rhs = fragment->getRhs()->term2Dag();
    if (lhs->equal(rhs))
      continue;
    //
    //	Generate an SMT clause.
    //
    DagNode *clause;
    if (rhs->symbol() == smtInfo.getTrueSymbol())
      clause = lhs; // optimize QF = true
    else if (lhs->symbol() == smtInfo.getTrueSymbol())
      clause = rhs; // optimize true = QF
    else
    {
      Symbol *eqOp = smtInfo.getEqualityOperator(lhs, rhs);
      if (eqOp == 0)
      {
        IssueWarning(*(fragment->getLhs()) << ": no SMT equality operator available for condition fragment " << cf);
        continue;
      }
      args[0] = lhs;
      args[1] = rhs;
      clause = eqOp->makeDagNode(args);
    }
    //
    //	Conjunct with existing constraint.
    //
    if (constraint == 0)
      constraint = clause;
    else
    {
      args[0] = constraint;
      args[1] = clause;
      constraint = smtInfo.getConjunctionOperator()->makeDagNode(args);
    }
  }
  //
  //	Default to true.
  //
  return constraint == 0 ? smtInfo.getTrueSymbol()->makeDagNode() : constraint;
}