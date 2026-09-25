//
//	Pattern candidate index for constrained-state folding.
//
#ifndef _folder_hh_
#define _folder_hh_
#include <map>
#include <vector>
#include "simpleRootContainer.hh"

class Folder : private SimpleRootContainer
{
  NO_COPYING(Folder);

public:
  Folder(bool fold);
  ~Folder();

  void addState(int index, DagNode *state, int parentIndex);
  void findSubsumers(DagNode *state, std::vector<int> &indices) const;

private:
  struct RetainedState
  {
    RetainedState(DagNode *state, int parentIndex, bool fold);
    ~RetainedState();
    bool subsumes(DagNode *state) const;

    DagNode *const state;
    const int parentIndex;
    int depth;
    //
    //	Only used for folding.
    //
    Term *stateTerm;
    LhsAutomaton *matchingAutomaton;
    int nrMatchingVariables; // number of variables needed for matching; includes
                             // any abstraction variables
  };

  typedef map<int, RetainedState *> RetainedStateMap;
  void markReachableNodes();

  const bool fold;
  // SMT constraints decide whether a state can be discarded, so pattern
  // matching alone must never evict an entry from this index.
  RetainedStateMap retainedStates;
};

#endif
