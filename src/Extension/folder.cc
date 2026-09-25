//
//	Implementation for class Folder.
//

//	utility stuff
#include "macros.hh"
#include "vector.hh"

//	forward declarations
#include "interface.hh"
#include "core.hh"

//	interface class definitions
#include "symbol.hh"
#include "dagNode.hh"
#include "term.hh"
#include "lhsAutomaton.hh"
#include "subproblem.hh"

//	core class definitions
#include "rewritingContext.hh"
#include "variableInfo.hh"
#include "subproblemAccumulator.hh"

//	higher class definitions
#include "folder.hh"

Folder::Folder(bool fold)
    : fold(fold)
{
}

Folder::~Folder()
{
  for (auto &i : retainedStates)
    delete i.second;
}

void Folder::markReachableNodes()
{
  for (auto &i : retainedStates)
  {
    i.second->state->mark();
  }
}

void Folder::addState(int index, DagNode *state, int parentIndex)
{
  Verbose("new state " << index << " added");
  RetainedState *newState = new RetainedState(state, parentIndex, fold);
  int depth = 0;
  if (parentIndex != NONE)
  {
    RetainedStateMap::const_iterator j = retainedStates.find(parentIndex);
    if (j == retainedStates.end()){
      IssueWarning("assertion failed with " << parentIndex << " where its index is " << index);
    }
    Assert(j != retainedStates.end(), "couldn't find state with index " << parentIndex);
    depth = j->second->depth + 1;
  }
  newState->depth = depth;
  retainedStates.insert(RetainedStateMap::value_type(index, newState));
}

void Folder::findSubsumers(DagNode *state, std::vector<int> &indices) const
{
  if (!fold)
    return;
  for (const auto &entry : retainedStates)
    if (entry.second->subsumes(state))
      indices.push_back(entry.first);
}

Folder::RetainedState::RetainedState(DagNode *state, int parentIndex, bool fold)
    : state(state),
      parentIndex(parentIndex)
{
  if (fold)
  {
    //
    //	Make term version of state.
    //
    Term *t = state->symbol()->termify(state);
    //
    //	Even thoug t should be in normal form we need to set hash values.
    //
    t = t->normalize(true);
    VariableInfo variableInfo;
    t->indexVariables(variableInfo);
    t->symbol()->fillInSortInfo(t);
    t->analyseCollapses();

    NatSet boundUniquely;
    bool subproblemLikely;

    t->determineContextVariables();
    t->insertAbstractionVariables(variableInfo);

    matchingAutomaton = t->compileLhs(false, variableInfo, boundUniquely, subproblemLikely);
    stateTerm = t;
    nrMatchingVariables = variableInfo.getNrProtectedVariables(); // may also have some
                                                                  // abstraction variables
  }
  else
  {
    stateTerm = 0;
    matchingAutomaton = 0;
    nrMatchingVariables = 0;
  }
}

Folder::RetainedState::~RetainedState()
{
  delete matchingAutomaton;
  if (stateTerm)
    stateTerm->deepSelfDestruct();
}

bool Folder::RetainedState::subsumes(DagNode *state) const
{
  MemoryCell::okToCollectGarbage(); // otherwise we have huge accumulation of junk from matching
  int nrSlotsToAllocate = nrMatchingVariables;
  if (nrSlotsToAllocate == 0)
    nrSlotsToAllocate = 1; // substitutions subject to clear() must always have at least one slot

  RewritingContext matcher(nrSlotsToAllocate);
  matcher.clear(nrMatchingVariables);
  Subproblem *subproblem = 0;

  bool result = matchingAutomaton->match(state, matcher, subproblem) &&
                (subproblem == 0 || subproblem->solve(true, matcher));
  delete subproblem;
  return result;
}
