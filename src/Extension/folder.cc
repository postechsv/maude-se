//
//	Implementation for class Folder.
//

//	utility stuff
#include "macros.hh"
#include "vector.hh"
#include <set>

//	forward declarations
#include "interface.hh"
#include "core.hh"
class VariantSearch;

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
#include "narrowingFolder.hh"

//	higher class definitions
#include "folder.hh"

Folder::Folder(bool fold)
    : fold(fold),
      // The context and generator are only used by variant folding (vfold).
      patternIndex(fold ? new NarrowingFolder(0, 0, true, false, false) : 0)
{
}

Folder::~Folder()
{
  delete patternIndex;
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
  if (!fold)
    return;

  // The official Maude folder maintains the antichain of most-general
  // representative patterns. Its pattern-only rejection/eviction is safe
  // here because every constrained state remains in retainedStates.
  if (!patternIndex->insertState(index, state, NONE, NONE))
  {
    // Only a rejected pattern needs classification under an existing root.
    // An equivalent pattern keeps its own constraint as a separate child.
    int coveringRoot = NONE;
    for (const auto &root : roots)
    {
      RetainedState *representative = retainedStates.find(root.first)->second;
      if (!representative->subsumes(state))
        continue;
      if (coveringRoot == NONE)
        coveringRoot = root.first;
      for (int groupIndex : root.second)
      {
        RetainedState *group = retainedStates.find(groupIndex)->second;
        if (group->subsumes(state) && newState->subsumes(group->state))
        {
          groups.find(groupIndex)->second.push_back(index);
          newState->releaseMatcher();
          return;
        }
      }
    }
    Assert(coveringRoot != NONE, "Maude folder rejected a pattern without a covering root");
    groups[index].push_back(index);
    roots.find(coveringRoot)->second.push_back(index);
    return;
  }

  // A newly admitted pattern may evict older roots from the official folder.
  // Move their groups under it without removing any constrained states.
  groups[index].push_back(index);
  Vector<DagNode *> mostGeneral = patternIndex->getMostGeneralStates();
  std::set<DagNode *> surviving(mostGeneral.begin(), mostGeneral.end());
  std::vector<int> absorbed;
  std::vector<int> &newRootGroups = roots[index];
  newRootGroups.push_back(index);
  for (const auto &root : roots)
  {
    if (root.first != index &&
        surviving.find(retainedStates.find(root.first)->second->state) == surviving.end())
    {
      newRootGroups.insert(newRootGroups.end(), root.second.begin(), root.second.end());
      absorbed.push_back(root.first);
    }
  }
  for (int oldRoot : absorbed)
    roots.erase(oldRoot);
  Assert(roots.size() == surviving.size(), "pattern groups disagree with Maude folder");
}

void Folder::findSubsumers(DagNode *state, std::vector<int> &indices) const
{
  if (!fold)
    return;
  for (const auto &root : roots)
  {
    if (!retainedStates.find(root.first)->second->subsumes(state))
      continue;
    for (int groupIndex : root.second)
    {
      if (!retainedStates.find(groupIndex)->second->subsumes(state))
        continue;
      const std::vector<int> &members = groups.find(groupIndex)->second;
      indices.insert(indices.end(), members.begin(), members.end());
    }
  }
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
  releaseMatcher();
}

void Folder::RetainedState::releaseMatcher()
{
  delete matchingAutomaton;
  matchingAutomaton = 0;
  if (stateTerm)
    stateTerm->deepSelfDestruct();
  stateTerm = 0;
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
