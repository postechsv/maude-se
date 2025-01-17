//
//	Class for searching for sequences of rewrites within a DAG.
//
#ifndef _rewriteSmtSequenceSearch_hh_
#define _rewriteSmtSequenceSearch_hh_
#include "natSet.hh"
#include "sequenceSearch.hh"
#include "smtStateTransitionGraph.hh"
#include "pattern.hh"
#include "matchSearchState.hh"
#include "simpleRootContainer.hh"

class RewriteSmtSequenceSearch : public SequenceSearch, public SmtStateTransitionGraph, private SimpleRootContainer
{
  NO_COPYING(RewriteSmtSequenceSearch);

public:
  RewriteSmtSequenceSearch(RewritingContext *initial,
                           SearchType searchType,
                           Pattern *goal, Pattern *smtGoal,
                           const SMT_Info &smtInfo,
                           SMT_EngineWrapper *engine,
                           FreshVariableGenerator *freshVariableGenerator,
                           bool fold = true, bool merge = false,
                           int maxDepth = -1,
                           const mpz_class &avoidVariableNumber = 0);
  ~RewriteSmtSequenceSearch();

  bool findNextMatch();
  //
  //	Information particular to most recent match.
  //
  const Substitution *getSubstitution() const;
  DagNode* getFinalConstraint();  // conjunction of constraints from state and constraints from match
  const mpz_class& getMaxVariableNumber() const;  // largest fresh variable appearing in substitution or constraint
  const NatSet& getSMT_VarIndices() const;

  const Pattern *getGoal() const;
  Rule *getStateRule(int stateNr) const;
  int getStateNr() const;

  VariableInfo* getVariableInfo();

private:
  int findNextInterestingState();

  DagNode *makeConstraintFromCondition(
      const Vector<ConditionFragment *> &condition);

  void findSMT_Variables();
  bool checkMatchConstraint(int stateNr);
  void markReachableNodes();

  typedef map<int, DagNode *> SMT_VarDags;

  //
  //	Information abound SMT variables in target, computed at initialization.
  //
  NatSet smtVarIndices;
  SMT_VarDags smtVarDags;

  mpz_class newVariableNumber;

  Pattern *const goal;
  Pattern *const smtGoal;
  const int maxDepth;
  int explore;
  int exploreDepth;
  int firstDeeperNodeNr;
  int nextArc;
  bool needToTryInitialState;
  bool reachingInitialStateOK;
  bool normalFormNeeded;
  MatchSearchState *matchState;
  int stateNr;

  double time;

  DagNode* smtGoalConst;
};

inline const Pattern *
RewriteSmtSequenceSearch::getGoal() const
{
  return goal;
}

inline const Substitution *
RewriteSmtSequenceSearch::getSubstitution() const
{
  return matchState->getContext();
}

inline VariableInfo*
RewriteSmtSequenceSearch::getVariableInfo(){
  return matchState->getPattern();
}

inline int
RewriteSmtSequenceSearch::getStateNr() const
{
  return stateNr;
}

inline DagNode *
RewriteSmtSequenceSearch::getFinalConstraint()
{
  // TODO
  SmtTerm* finalConstTerm = this->getStateConst(stateNr);
  DagNode* finalConst = conv->term2dag(finalConstTerm);

  finalConst->computeTrueSort(*initial);
  return finalConst;
}

inline const mpz_class&
RewriteSmtSequenceSearch::getMaxVariableNumber() const
{
  // TODO: should update with a new interface
  return seen[stateNr]->avoidVariableNumber;
}

inline const NatSet&
RewriteSmtSequenceSearch::getSMT_VarIndices() const
{
  return smtVarIndices;
}
#endif
