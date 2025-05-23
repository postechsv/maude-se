//
//	Class for building a state transition graph on-the-fly, with hash consing.
//
#ifndef _smtStateTransitionGraph_hh_
#define _smtStateTransitionGraph_hh_
#include <set>
#include <map>
#include "SMT_Info.hh"
#include "ext.hh"
#include "hashConsSet.hh"
#include "rewritingContext.hh"
// #include "narrowingFolder.hh"
#include "folder.hh"

#include "variableDagNode.hh"
#include "SMT_EngineWrapper.hh"

#include "smtInterface.hh"
#include <ctime>

class SmtStateTransitionGraph
{
  NO_COPYING(SmtStateTransitionGraph);

public:
  typedef map<int, set<Rule *>> ArcMap;
  typedef map<DagNode *, DagNode *> Mapping;

  SmtStateTransitionGraph(RewritingContext *initial,
                          const SMT_Info &smtInfo, SMT_EngineWrapper *engine,
                          FreshVariableGenerator *freshVariableGenerator, 
                          bool fold, bool merge,
                          const mpz_class &avoidVariableNumber = 0);
  ~SmtStateTransitionGraph();

  int getNrStates() const;
  int getNextState(int stateNr, int index);
  DagNode *getStateDag(int stateNr);
  DagNode *getStateConstDag(int stateNr);
  SmtTerm getStateConst(int stateNr);
  std::map<DagNode*, DagNode*>* getStateModel(int stateNr);
  int getStateDepth(int stateNr) const;
  const ArcMap &getStateFwdArcs(int stateNr) const;
  //
  //	Stuff needed for search.
  //
  RewritingContext *getContext();
  void transferCountTo(RewritingContext &recipient);
  int getStateParent(int stateNr) const;

protected:
  struct State
  {
    State(int hashConsIndex, int parent);
    const int parent;

    int constTermIndex;

    mpz_class avoidVariableNumber;
    const int hashConsIndex;
    RewriteSmtSearchState *rewriteState;

    Vector<int> nextStates;
    bool fullyExplored;
    ArcMap fwdArcs;

    DagNode *dag;
    int depth;
  };

  struct ConstrainedTerm
  {
    ConstrainedTerm(DagNode *dag, SmtTerm constraint);
    ~ConstrainedTerm();

    DagNode *dag;
    SmtTerm constraint;
    SmtModel model;
    // for matching
    VariableInfo variableInfo;
    Term *term;
    LhsAutomaton *matchingAutomaton;
    int nrMatchingVariables; // number of variables needed for matching; includes any abstraction variables

    bool findMatching(DagNode *other, Converter converter, Connector connector);
    TermSubst subst;
  };

  bool fold;
  bool merge;

  // key: State index
  typedef map<int, Vector<ConstrainedTerm *>> ConstrainedTermMap;

  // state index , const id?
  typedef tuple<int, int> StateId;
  typedef map<StateId, int> Map2Seen;

  // void insertNewState(int parent);
  const SMT_Info &smtInfo;         // information about SMT sort; might get folded into wrapper
  SMT_EngineWrapper *const engine; // wrapper to call the SMT engine
  FreshVariableGenerator *freshVariableGenerator;

  State *initState;
  int counter;
  RewritingContext *initial;

  ConstrainedTermMap consTermSeen;
  Vector<State *> seen;

  // Vector<int> hashCons2seen;  // partial map of hashCons indices to state indices
  // HashConsSet hashConsSet;
  Map2Seen map2seen;

  Folder stateCollection;

protected:
  void printStateConst(int depth);

protected:
  Converter conv;
  Connector connector;
  Connector connector2;

  //
  // typedef map<const char *, PyObject *> SortMap;
  // typedef map<const char *, PyObject *> FuncMap;

  // SortMap sortMap;
  // FuncMap funcMap;

  double nextTime;
  double rewriteTime;
  double elseTime;

public:
  // Aux function
  // VariableInfo* getVariableInfo(int stateNr);
};

inline SmtStateTransitionGraph::State::State(int hashConsIndex, int parent)
    : hashConsIndex(hashConsIndex),
      parent(parent)
{
  rewriteState = 0;
  fullyExplored = false;
}

inline int
SmtStateTransitionGraph::getNrStates() const
{
  return seen.size();
}

inline DagNode *
SmtStateTransitionGraph::getStateDag(int stateNr)
{
  // TODO: return const DAG
  if (seen.size() <= stateNr)
  {
    IssueWarning("not found in seen states");
  }

  State *state = seen[stateNr];

  if (consTermSeen[state->hashConsIndex].size() <= state->constTermIndex)
  {
    IssueWarning("consTermseen length wrong");
  }
  ConstrainedTerm *ct = consTermSeen[state->hashConsIndex][state->constTermIndex];
  return ct->dag;
}

inline SmtTerm
SmtStateTransitionGraph::getStateConst(int stateNr)
{
  // TODO: return const DAG
  if (seen.size() <= stateNr)
  {
    IssueWarning("not found in seen states");
  }

  State *state = seen[stateNr];

  if (consTermSeen[state->hashConsIndex].size() <= state->constTermIndex)
  {
    IssueWarning("consTermseen length wrong");
  }
  ConstrainedTerm *ct = consTermSeen[state->hashConsIndex][state->constTermIndex];
  return ct->constraint;
}

inline DagNode *
SmtStateTransitionGraph::getStateConstDag(int stateNr)
{
  // TODO
  SmtTerm constTerm = getStateConst(stateNr);
  DagNode* constDag = conv->term2dag(constTerm);

  constDag->computeTrueSort(*initial);
  return constDag;
}

// inline VariableInfo*
// SmtStateTransitionGraph::getVariableInfo(int stateNr)
// {
//   // TODO: return const DAG
//   if (seen.size() <= stateNr)
//   {
//     IssueWarning("not found in seen states");
//   }

//   State *state = seen[stateNr];

//   if (consTermSeen[state->hashConsIndex].size() <= state->constTermIndex)
//   {
//     IssueWarning("consTermseen length wrong");
//   }
//   ConstrainedTerm *ct = consTermSeen[state->hashConsIndex][state->constTermIndex];
//   return &ct->variableInfo;
// }

inline std::map<DagNode*, DagNode*>*
SmtStateTransitionGraph::getStateModel(int stateNr)
{
  // TODO: return const DAG
  if (seen.size() <= stateNr)
  {
    IssueWarning("not found in seen states");
  }

  State *state = seen[stateNr];

  if (consTermSeen[state->hashConsIndex].size() <= state->constTermIndex)
  {
    IssueWarning("consTermseen length wrong");
  }
  ConstrainedTerm *ct = consTermSeen[state->hashConsIndex][state->constTermIndex];
  std::map<DagNode*, DagNode*>* modelMap = new std::map<DagNode*, DagNode*>();
  if (ct->model == nullptr){
    IssueWarning("bug occurred");
  }

  SmtTermVector ks = ct->model->keys();

  for (auto &elem : *ks){
    DagNode* t = conv->term2dag(elem);
    DagNode* v = conv->term2dag(ct->model->get(elem));

    t->computeTrueSort(*initial);
    v->computeTrueSort(*initial);

    // cout << tD << " ---> " << tV << endl;
    // (*modelMap)[tD] = tV;
    modelMap->insert(std::pair<DagNode*, DagNode*>(t, v));
  }

  return modelMap;
}

inline int SmtStateTransitionGraph::getStateDepth(int stateNr) const
{
  return seen[stateNr]->depth;
}

inline const SmtStateTransitionGraph::ArcMap &
SmtStateTransitionGraph::getStateFwdArcs(int stateNr) const
{
  return seen[stateNr]->fwdArcs;
}

inline RewritingContext *
SmtStateTransitionGraph::getContext()
{
  return initial;
}

inline void
SmtStateTransitionGraph::transferCountTo(RewritingContext &recipient)
{
  recipient.transferCountFrom(*initial);
}

inline int
SmtStateTransitionGraph::getStateParent(int stateNr) const
{
  return seen[stateNr]->parent;
}

#endif
