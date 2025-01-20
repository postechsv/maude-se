#ifndef ABSTRACT_SMT_MANAGER_HH
#define ABSTRACT_SMT_MANAGER_HH

// utility stuff
#include "macros.hh"
#include "vector.hh"

// forward declarations
#include "interface.hh"
#include "core.hh"
#include "freeTheory.hh"
#include "variable.hh"
#include "builtIn.hh"
#include "mixfix.hh"

// interface class definitions
#include "symbol.hh"
#include "term.hh"

// core class definitions
#include "rewritingContext.hh"
#include "symbolMap.hh"

// variable class definitions
#include "variableSymbol.hh"
#include "variableDagNode.hh"

// free theory class definitions
#include "freeDagNode.hh"


// builtin class definition
#include "bindingMacros.hh"

#include "SMT_Info.hh"
#include "SMT_Symbol.hh"
#include "SMT_NumberSymbol.hh"
#include "SMT_NumberDagNode.hh"
#include "SMT_EngineWrapper.hh"


#include "extensionSymbol.hh"
#include "smtCheckSymbol.hh"
#include "tacticApplySymbol.hh"
#include "smtInterface.hh"
#include <map>

class SmtCheckerSymbol;
class TacticApplySymbol;

class ExtensionException {
private:
    const char* exception;
public:
    ExtensionException(const char* exc){
        this->exception = exc;
    }
    const char* c_str(){
        return this->exception;
    }
};

/*
 * T is expression type
 *
 * U is struct type that contains user define comparator for
 * expression object.
 *
 * V is type data structure.
 */
template <typename T, typename U>
class SmtEngineWrapperEx : virtual public SMT_EngineWrapper
{
public:
    enum MulType {
        AND,
        OR,
        INT_ADD,
        INT_SUB,
        INT_MUL,
        REAL_ADD,
        REAL_SUB,
        REAL_MUL,
    };

    enum ExprType {
        BOOL,
        INT,
        REAL,
        BUILTIN,
    };

protected:

    /*
     * SMT_EngineWrapper class contains
     */
    const SMT_Info& smtInfo;
    typedef map<DagNode *, T> SmtManagerVariableMap;
    typedef map<T, DagNode *, U> ReverseSmtManagerVariableMap;
    SmtManagerVariableMap smtManagerVariableMap;
    const char* smt_null_term = "SMT_NULL_TERM";

    inline bool isNull(const char* c_str){
        return !strcmp(c_str, smt_null_term);
    }

    inline void SMT_NULL_TERM () noexcept(false) {
        throw ExtensionException(smt_null_term);
    }

    inline void incrFormulaSize() {
        formulaSize += 1;
    }

    inline void decrFormulaSize() {
        formulaSize -= 1;
    }

    inline void resetFormulaSize() {
        formulaSize = 0;
    }

    /*
     * hasVariable is used to check whether to make reverseVariableMap
     */
    bool hasVariable;
    uint64_t formulaSize;

public:

    SmtEngineWrapperEx(const SMT_Info& smtInfo):
    smtInfo(smtInfo), formulaSize(0) {}

    virtual ~SmtEngineWrapperEx(){
        smtManagerVariableMap.clear();
    }

    /*
     * Dag2Term should throw ExtensionException when any error occurs.
     *
     * makeExtensionVariable : User should check smtCheckerSymbol is null or not.
     * variableGenerator should know its dag parameter type before calling.
     * checkDagExtension's result is different type compare to SMT_EngineWrapper result type.
     */
    virtual DagNode* Term2Dag(T exp, ExtensionSymbol* extensionSymbol, ReverseSmtManagerVariableMap* rsv) noexcept(false) = 0;
    virtual T Dag2Term(DagNode* dag, ExtensionSymbol* extensionSymbol) noexcept(false) = 0;
    virtual DagNode* generateAssignment(DagNode* dagNode,ExtensionSymbol* extensionSymbol) = 0;
    virtual DagNode* simplifyDag(DagNode* dagNode, ExtensionSymbol* extensionSymbol)= 0;
    virtual DagNode* applyTactic(DagNode* dagNode, DagNode* tacticTypeDagNode, ExtensionSymbol* extensionSymbol) = 0;
    virtual T variableGenerator(DagNode* dag, ExprType exprType) = 0;
    virtual Connector* getConnector() = 0;
    virtual Converter* getConverter() = 0;
    // virtual Result checkDagContextFree(DagNode* dag, ExtensionSymbol* extensionSymbol) = 0;

protected:

    DagNode * multipleGen(Vector<DagNode*>* dags, int i, MulType type, ExtensionSymbol* extensionSymbol){
        Vector < DagNode* > arg(2);
        arg[0] = (*dags)[i];
        if (i == dags->length() - 1){
            incrFormulaSize();
            return arg[0];
        }
        arg[1] = multipleGen(dags, i+1, type, extensionSymbol);
        switch(type){
            case MulType::AND:
                incrFormulaSize();
                return extensionSymbol->andBoolSymbol->makeDagNode(arg);
            case MulType::OR:
                incrFormulaSize();
                return extensionSymbol->orBoolSymbol->makeDagNode(arg);
            case MulType::INT_ADD:
                incrFormulaSize();
                return extensionSymbol->plusIntSymbol->makeDagNode(arg);
            case MulType::INT_SUB:
                incrFormulaSize();
                return extensionSymbol->minusIntSymbol->makeDagNode(arg);
            case MulType::INT_MUL:
                incrFormulaSize();
                return extensionSymbol->mulIntSymbol->makeDagNode(arg);
            case MulType::REAL_ADD:
                incrFormulaSize();
                return extensionSymbol->plusRealSymbol->makeDagNode(arg);
            case MulType::REAL_SUB:
                incrFormulaSize();
                return extensionSymbol->minusRealSymbol->makeDagNode(arg);
            case MulType::REAL_MUL:
                incrFormulaSize();
                return extensionSymbol->mulRealSymbol->makeDagNode(arg);
        }
    }

    // This function covers the previous makeBooleanExpr implementation.
    T makeExpr(DagNode *dag, ExtensionSymbol* extensionSymbol, bool isBooleanExpr) noexcept(false) {
        //	Checks if a dag actually represents a Boolean expression
        //	and convert it to a term representation.
        if (VariableDagNode * v = dynamic_cast<VariableDagNode *>(dag)) {
            Sort *rangeSort = v->symbol()->getRangeSort();
            if (smtInfo.getType(rangeSort) == SMT_Info::BOOLEAN) {
                return variableGenerator(dag, ExprType::BUILTIN);
            }
        }
        if (isBooleanExpr) {
            if (SMT_Symbol* s = dynamic_cast<SMT_Symbol*>(dag->symbol())){
                Sort* rangeSort = s->getRangeSort();
                if (smtInfo.getType(rangeSort) == SMT_Info::BOOLEAN){
                    try {
                        return Dag2Term(dag, extensionSymbol);
                    } catch (ExtensionException& ex) {
                        throw ex;
                    }
                }
            } else if (dag->symbol() == extensionSymbol->boolVarSymbol) {
                try {
                    return Dag2Term(dag, extensionSymbol);
                } catch (ExtensionException& ex) {
                    throw ex;
                }
            }
            IssueWarning("Expecting an SMT Boolean expression but saw but saw " << dag);
        } else {
            // For builtin variables such as b:Boolean, i:Integer, r:Real
            try{
                return Dag2Term(dag, extensionSymbol);
            } catch (ExtensionException& ex) {
                throw ex;
            }
        }
    }

    T makeExtensionVariable(DagNode *dag, ExtensionSymbol* extensionSymbol) noexcept(false) {
        if (extensionSymbol != nullptr) {
            if (dag->symbol() == extensionSymbol->boolVarSymbol) {
                return variableGenerator(dag, ExprType::BOOL);
            } else if (dag->symbol() == extensionSymbol->intVarSymbol) {
                return variableGenerator(dag, ExprType::INT);
            } else if (dag->symbol() == extensionSymbol->realVarSymbol) {
                return variableGenerator(dag, ExprType::REAL);
            }
        }
        return variableGenerator(dag, ExprType::BUILTIN);
    }

    ReverseSmtManagerVariableMap*
    generateReverseVariableMap(){
        ReverseSmtManagerVariableMap* rsv = new ReverseSmtManagerVariableMap();
        for (auto it = smtManagerVariableMap.begin(); it != smtManagerVariableMap.end(); it++) {
            (*rsv)[it->second] = it->first;
        }
        return rsv;
    }
};

#endif
