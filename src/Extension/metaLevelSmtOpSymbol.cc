//
//      Implementation for class MetaLevelSmtOpSymbol.
//

//      utility stuff
#include "macros.hh"
#include "vector.hh"
#include "pointerMap.hh"
#include "meta.hh"

//      forward declarations
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "higher.hh"
#include "freeTheory.hh"
#include "AU_Theory.hh"
#include "NA_Theory.hh"
#include "builtIn.hh"
#include "strategyLanguage.hh"
#include "mixfix.hh"
#include "SMT.hh"

//      interface class definitions
#include "symbol.hh"
#include "dagNode.hh"
#include "rawDagArgumentIterator.hh"
#include "rawArgumentIterator.hh"
#include "term.hh"
#include "extensionInfo.hh"

//      core class definitions
#include "variableInfo.hh"
#include "variableSymbol.hh"
#include "preEquation.hh"
#include "substitution.hh"
#include "rewritingContext.hh"
#include "module.hh"
#include "label.hh"
#include "rule.hh"
#include "symbolMap.hh"

//	higher class definitions
#include "pattern.hh"
#include "rewriteSearchState.hh"
#include "matchSearchState.hh"
#include "rewriteSequenceSearch.hh"
#include "narrowingSequenceSearch.hh"
#include "unificationProblem.hh"
#include "irredundantUnificationProblem.hh"
#include "variantSearch.hh"
#include "filteredVariantUnifierSearch.hh"
#include "narrowingSearchState2.hh"
#include "narrowingSequenceSearch3.hh"

//      free theory class definitions
#include "freeNet.hh"
#include "freeSymbol.hh"
#include "freeDagNode.hh"

//      variable class definitions
#include "variableDagNode.hh"

//      built in class definitions
#include "succSymbol.hh"
#include "bindingMacros.hh"

//      front end class definitions
#include "userLevelRewritingContext.hh"
#include "quotedIdentifierSymbol.hh"
#include "quotedIdentifierDagNode.hh"
#include "quotedIdentifierOpSymbol.hh"
#include "metaModule.hh"
#include "metaLevel.hh"
#include "metaLevelSmtOpSymbol.hh"
#include "fileTable.hh"
#include "syntacticPreModule.hh"
#include "interpreter.hh"
#include "visibleModule.hh"
#include "freshVariableSource.hh"
#include "mixfixParser.hh"

//	our stuff
#include "metaLevelOpSymbol.hh"
#include "metaSmtSearch.cc"
#include "metaSmtCheck.cc"


MetaLevelSmtOpSymbol::MetaLevelSmtOpSymbol(int id, int nrArgs, const Vector<int> &strategy)
    : FreeSymbol(id, nrArgs, strategy)
{
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
  SymbolName = 0;
#include "metaLevelSmtSignature.cc"
#undef MACRO

  metaLevel = 0;
  shareWith = 0;

  trueTerm = 0;
  falseTerm = 0;
}

MetaLevelSmtOpSymbol::~MetaLevelSmtOpSymbol()
{
  // if (shareWith == 0)
    // delete metaLevel;
}

MetaLevelSmtOpSymbol::AliasMapParserPair::~AliasMapParserPair()
{
  delete parser;
}

bool MetaLevelSmtOpSymbol::okToBind()
{
  // if (shareWith != 0)
  //   return false;
  if (metaLevel == 0)
    metaLevel = new MetaLevel;
  return true;
}

bool MetaLevelSmtOpSymbol::attachData(const Vector<Sort *> &opDeclaration,
                                   const char *purpose,
                                   const Vector<const char *> &data)
{
  // id-hook < purpose > (data)
// cout << "attachData is called" << endl;
// cout << "arity: " << arity() << endl;
// cout << "Data length: " << data.length() << endl;

  if (data.length() == 1)
  {
    const char *opName = data[0];
    // cout << "opName : " << opName <<endl;
#define MACRO(SymbolName, NrArgs) \
  if (arity() == NrArgs && strcmp(opName, #SymbolName) == 0) \
    descentFunction = &MetaLevelSmtOpSymbol::SymbolName; else
#include "descentSmtSignature.cc"
#undef MACRO
      return FreeSymbol::attachData(opDeclaration, purpose, data);
    return true;
  }
  return FreeSymbol::attachData(opDeclaration, purpose, data);
}

bool MetaLevelSmtOpSymbol::attachSymbol(const char *purpose, Symbol *symbol)
{
  // cout << "MetaLevelSmtOpSymbol attachSymbol" << endl;
  // cout << "with : " << symbol << endl;
  // cout << "purpose : " << purpose << endl;
  BIND_SYMBOL(purpose, symbol, shareWith, MetaLevelOpSymbol *);
  okToBind();

  // op-hook < purpose > (< symbol >)
  // attach symbol to current class's internal member

  Assert(symbol != 0, "null symbol for " << purpose);
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
  if (strcmp(purpose, #SymbolName) == 0) SymbolName = static_cast<SymbolClass*>(symbol); else
#include "metaLevelSmtSignature.cc"
#undef MACRO
    {
      IssueWarning("unrecognized symbol hook name " << QUOTE(purpose) << '.');
      return false;
    }
  return true;
}

bool MetaLevelSmtOpSymbol::attachTerm(const char *purpose, Term *term)
{
  BIND_TERM(purpose, term, trueTerm);
  BIND_TERM(purpose, term, falseTerm);
  return FreeSymbol::attachTerm(purpose, term);
}

void MetaLevelSmtOpSymbol::copyAttachments(Symbol *original, SymbolMap *map)
{
  // this function is used to copy descent function without calling attachData or attachSymbol
  if (shareWith == 0){
    MetaLevelSmtOpSymbol *orig = safeCast(MetaLevelSmtOpSymbol *, original);
    // metaLevel = new MetaLevl;
	  // metaLevel = new MetaLevel(orig->metaLevel, map);
    descentFunction = orig->descentFunction;
  // copy other symbols

#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
  COPY_SYMBOL(orig, SymbolName, map, SymbolClass*)
#include "metaLevelSmtSignature.cc"
#undef MACRO


  MetaLevelOpSymbol *sw = orig->shareWith;
    if (sw != 0)
    {
      metaLevel = 0;
      shareWith = (map == 0) ? sw : safeCast(MetaLevelOpSymbol *, map->translate(sw));
    }
    else
    {
      IssueWarning("this is impossible (another shareWith is null)");
      // metaLevel = new MetaLevel(orig->metaLevel, map);
      // shareWith = 0;
    }

    COPY_TERM(orig, trueTerm, map);
    COPY_TERM(orig, falseTerm, map);
  }
  FreeSymbol::copyAttachments(original, map);
}

void MetaLevelSmtOpSymbol::getDataAttachments(const Vector<Sort *> &opDeclaration,
                                           Vector<const char *> &purposes,
                                           Vector<Vector<const char *>> &data)
{
  FreeSymbol::getDataAttachments(opDeclaration, purposes, data);
}

void MetaLevelSmtOpSymbol::getSymbolAttachments(Vector<const char *> &purposes,
                                             Vector<Symbol *> &symbols)
{
  FreeSymbol::getSymbolAttachments(purposes, symbols);
}

void MetaLevelSmtOpSymbol::getTermAttachments(Vector<const char *> &purposes,
                                           Vector<Term *> &terms)
{
  APPEND_TERM(purposes, terms, trueTerm);
  APPEND_TERM(purposes, terms, falseTerm);
  FreeSymbol::getTermAttachments(purposes, terms);
}

void MetaLevelSmtOpSymbol::postInterSymbolPass()
{
  if (shareWith == 0){
    metaLevel->postInterSymbolPass();
  }
  else {
    metaLevel = shareWith->getMetaLevel();
  }
PREPARE_TERM(trueTerm);
PREPARE_TERM(falseTerm);
}

void MetaLevelSmtOpSymbol::reset()
{
  if (shareWith == 0 && metaLevel != 0){
    metaLevel->reset();
  }
  trueTerm.reset();
  falseTerm.reset();
}

bool MetaLevelSmtOpSymbol::eqRewrite(DagNode *subject, RewritingContext &context)
{
  Assert(this == subject->symbol(), "bad symbol");
  Assert(metaLevel != 0, "metaLevel not set for " << this << " during postInterSymbolPass()");

  FreeDagNode *d = safeCast(FreeDagNode *, subject);
  if (standardStrategy())
  {
    const int nrArgs = arity();
    for (int i = 0; i < nrArgs; ++i){
      d->getArgument(i)->reduce(context);
    }
    return (this->*descentFunction)(d, context) || FreeSymbol::eqRewrite(subject, context);
  }
  return false;
}

RewritingContext*
MetaLevelSmtOpSymbol::term2RewritingContext(Term* term, RewritingContext& context)
{
  term = term->normalize(false);
  DagNode* d = term->term2DagEagerLazyAware();
  term->deepSelfDestruct();
  return context.makeSubcontext(d, UserLevelRewritingContext::META_EVAL);
}

DagNode* MetaLevelSmtOpSymbol::upSmtAssn(MixfixModule* m, std::map<DagNode *, DagNode *> *model, PointerMap &qidMap, PointerMap &dagNodeMap){
  DagNode* smtAssn = emptySubstitutionSymbol->makeDagNode();
  for(auto &ij : *model){
    // cout << ij.first << " --> " << ij.second << endl;
    Vector<DagNode*> assnArgs(2);
    assnArgs[0] = metaLevel->upDagNode(ij.first, m, qidMap, dagNodeMap);
    assnArgs[1] = metaLevel->upDagNode(ij.second, m, qidMap, dagNodeMap);
    DagNode* assn = assignmentSymbol->makeDagNode(assnArgs);

    Vector<DagNode*> tmp(2);
    tmp[0] = assn;
    tmp[1] = smtAssn;

    smtAssn = substitutionSymbol->makeDagNode(tmp);
  }

  delete model;
  return smtAssn;
}

DagNode*
MetaLevelSmtOpSymbol::upSmtResult(
    DagNode* state,
    const Substitution& substitution,
    const VariableInfo& variableInfo,
    const NatSet& smtVariables,
    DagNode* constraint,
    const mpz_class& variableNumber,
    int stateNr,
    MixfixModule* m,
    std::map<DagNode*, DagNode*>* model)
{
  Assert(state != 0, "null state");
  Assert(constraint != 0, "null constraint");
  Assert(metaLevel != 0, "null metaLevel");
  Assert(model != 0, "null model");

  DagNode* tmp = metaLevel->upSmtResult(state, substitution, variableInfo, smtVariables, constraint, variableNumber, m);

  FreeDagNode* r = static_cast<FreeDagNode*>(tmp);

  // DagNode* lastElem = 
  // emptySubstitutionSymbol->makeDagNode();

  PointerMap qidMap;
  PointerMap dagNodeMap;

  // TODO: this is inefficent because we don't use the pointer map of upSmtResult.
  DagNode* matching = metaLevel->upSubstitution(substitution, variableInfo, m, qidMap, dagNodeMap);
  
  if (FreeDagNode* stateDag = static_cast<FreeDagNode*>(r->getArgument(0))){
    Vector<DagNode*> args(5);
    // we have to retrive a term having an original sort.
    args[0] = stateDag->getArgument(1);
    // args[1] = r->getArgument(1);
    args[1] = matching;
    args[2] = r->getArgument(2);
    args[3] = upSmtAssn(m, model, qidMap, dagNodeMap);
    args[4] = r->getArgument(3);
    args[5] = metaLevel->succSymbol->makeNatDag(stateNr);
    return smtResultSymbol->makeDagNode(args);
  } 
  
  IssueWarning("failed to get a state dag");
  return smtFailureSymbol->makeDagNode();
}

inline const char*
MetaLevelSmtOpSymbol::downLogic(DagNode* arg) const
{
  int qid;
  if (metaLevel->downQid(arg, qid))
    {
      return Token::name(qid);
    }
  return nullptr;
}

DagNode*
MetaLevelSmtOpSymbol::upTrace(RewriteSmtSequenceSearch& state, MixfixModule* m, int stateNr)
{
  if (stateNr < 0){
    stateNr = state.getStateNr();
  }

  Vector<int> steps;
  for (int i = stateNr; i != 0; i = state.getStateParent(i))
    steps.append(i);

  int nrSteps = steps.size();   
  if (nrSteps == 0)
    return nilTraceSymbol->makeDagNode();

  Vector<DagNode*> args(nrSteps + 1);
  PointerMap qidMap;
  PointerMap dagNodeMap;
  int j = 0;
  for (int i = nrSteps - 1; i >= 0; --i, ++j)
    args[j] = upTraceStep(state, steps[i], m, qidMap, dagNodeMap);
  
  args[nrSteps] = upTraceStepFinal(state, stateNr, m, qidMap, dagNodeMap); // this is non-standard

  Vector<DagNode*> r_args(2);
  r_args[0] = (nrSteps == 0) ? args[0] : traceSymbol->makeDagNode(args);
  // r_args[1] = metaLevel->upSubstitution(*state.getSubstitution(), *state.getVariableInfo(), m, qidMap, dagNodeMap);
  r_args[1] = upSmtAssn(m, state.getStateModel(stateNr), qidMap, dagNodeMap);

  return traceResultSymbol->makeDagNode(r_args);
}

DagNode*
MetaLevelSmtOpSymbol::upTraceStep(RewriteSmtSequenceSearch& state,
		       int stateNr,
		       MixfixModule* m,
		       PointerMap& qidMap,
		       PointerMap& dagNodeMap)
{
  static Vector<DagNode*> args(4);
  int parentNr = state.getStateParent(stateNr);
  DagNode* dagNode = state.getStateDag(parentNr);
  DagNode* constDagNode = state.getStateConstDag(parentNr);
  
  // remove top state constructor
  FreeDagNode *d = safeCast(FreeDagNode *, dagNode);

  args[0] = metaLevel->upDagNode(d->getArgument(0), m, qidMap, dagNodeMap);
  args[1] = metaLevel->upDagNode(constDagNode, m, qidMap, dagNodeMap);
  args[2] = metaLevel->upType(d->getArgument(0)->getSort(), qidMap);
  args[3] = metaLevel->upRl(state.getStateRule(stateNr), m, qidMap);
  return traceStepSymbol->makeDagNode(args);
}

// this is non-standard
DagNode*
MetaLevelSmtOpSymbol::upTraceStepFinal(RewriteSmtSequenceSearch& state,
		       int stateNr,
		       MixfixModule* m,
		       PointerMap& qidMap,
		       PointerMap& dagNodeMap)
{
  static Vector<DagNode*> args(3);
  DagNode* dagNode = state.getStateDag(stateNr);
  DagNode* constDagNode = state.getStateConstDag(stateNr);
  
  // remove top state constructor
  FreeDagNode *d = safeCast(FreeDagNode *, dagNode);

  args[0] = metaLevel->upDagNode(d->getArgument(0), m, qidMap, dagNodeMap);
  args[1] = metaLevel->upDagNode(constDagNode, m, qidMap, dagNodeMap);
  args[2] = metaLevel->upType(d->getArgument(0)->getSort(), qidMap);
  return traceStepNoRlSymbol->makeDagNode(args);
}

DagNode*
MetaLevelSmtOpSymbol::upFailureTrace()
{
  return failureTraceSymbol->makeDagNode();
}