//
//      Class for symbols for built-in meta-level operations.
//
#ifndef _metaLevelSmtOpSymbol_hh_
#define _metaLevelSmtOpSymbol_hh_
#include "freeSymbol.hh"
#include "sequenceSearch.hh"
#include "metaModule.hh"
#include "userLevelRewritingContext.hh"
#include "ext.hh"

class MetaLevelSmtOpSymbol : public FreeSymbol
{
  NO_COPYING(MetaLevelSmtOpSymbol);

public:
  //
  //	This class exists so that we can cache a variable alias
  //	map together with a corresponding parser in a MetaLevelOpCache.
  //
  class AliasMapParserPair : public CacheableState
  {
    AliasMapParserPair();
    ~AliasMapParserPair();

    MixfixModule::AliasMap aliasMap;
    MixfixParser *parser;
    friend class MetaLevelSmtOpSymbol;
    friend class InterpreterManagerSymbol;
  };

  MetaLevelSmtOpSymbol(int id, int nrArgs, const Vector<int> &strategy);
  ~MetaLevelSmtOpSymbol();

  bool attachData(const Vector<Sort *> &opDeclaration,
                  const char *purpose,
                  const Vector<const char *> &data);
  bool attachSymbol(const char *purpose, Symbol *symbol);
  bool attachTerm(const char *purpose, Term *term);
  void copyAttachments(Symbol *original, SymbolMap *map);
  void getDataAttachments(const Vector<Sort *> &opDeclaration,
                          Vector<const char *> &purposes,
                          Vector<Vector<const char *>> &data);
  void getSymbolAttachments(Vector<const char *> &purposes,
                            Vector<Symbol *> &symbols);
  void getTermAttachments(Vector<const char *> &purposes,
                          Vector<Term *> &terms);

  void postInterSymbolPass();
  void reset();
  bool eqRewrite(DagNode *subject, RewritingContext &context);

  MetaLevel *getMetaLevel() const;

private:
  typedef bool (MetaLevelSmtOpSymbol::*DescentFunctionPtr)(FreeDagNode *subject, RewritingContext &context);

  static RewritingContext *term2RewritingContext(Term *term, RewritingContext &context);

  RewriteSmtSequenceSearch *make_RewriteSmtSequenceSearch(MetaModule *m,
                                                          FreeDagNode *subject,
                                                          RewritingContext &context) const;

  DagNode* upSmtResult(DagNode* state,
      const Substitution& substitution,
      const VariableInfo& variableInfo,
      const NatSet& smtVariables,
      DagNode* constraint,
      const mpz_class& variableNumber,
      MixfixModule* m, std::map<DagNode*, DagNode*>* model);

  const char* downLogic(DagNode* arg) const;

  bool okToBind();

  DagNode* make_model(VariableGenerator* vg, MetaModule* m);

  //
  //	Descent signature (generated by macro expansion).
  //
#define MACRO(SymbolName, NrArgs) \
  bool SymbolName(FreeDagNode* subject, RewritingContext& context);
#include "descentSmtSignature.cc"
#undef MACRO

  //
  //	Metalevel signature (generated by macro expansion).
  //
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
  SymbolClass* SymbolName;
#include "metaLevelSmtSignature.cc"
#undef MACRO

  DescentFunctionPtr descentFunction;
  MetaLevel *metaLevel;
  MetaLevelOpSymbol* shareWith;
};

inline MetaLevel *
MetaLevelSmtOpSymbol::getMetaLevel() const
{
  Assert(metaLevel != 0, "null metaLevel");
  return metaLevel;
}

inline MetaLevelSmtOpSymbol::AliasMapParserPair::AliasMapParserPair()
{
  parser = 0;
}

#endif
