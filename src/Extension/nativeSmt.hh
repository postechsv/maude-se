#ifndef NATIVE_SMT_HH
#define NATIVE_SMT_HH

// utility stuff
// #include "macros.hh"
// #include "vector.hh"

// forward declarations
// #include "interface.hh"
// #include "core.hh"
// #include "freeTheory.hh"
// #include "variable.hh"
// #include "builtIn.hh"
// #include "mixfix.hh"

// interface class definitions
// #include "symbol.hh"
// #include "term.hh"

// // core class definitions
// #include "rewritingContext.hh"
// #include "symbolMap.hh"

// variable class definitions
// #include "variableSymbol.hh"
// #include "variableDagNode.hh"

// free theory class definitions
#include "freeDagNode.hh"

#include "SMT_Info.hh"

// #include "extensionSymbol.hh"
// #include <map>

class MetaLevelSmtOpSymbol;

/*
 * Native Abstract class for SMT Converter
 */
template < typename T, typename U >
class NativeSmtConverter
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
     */
    const SMT_Info& smtInfo;
    typedef std::map<DagNode*, T> SmtManagerVariableMap;
    typedef std::map<T, DagNode*, U> ReverseSmtManagerVariableMap;

protected:

    SmtManagerVariableMap smtManagerVariableMap;
    MetaLevelSmtOpSymbol* extensionSymbol;

    /*
     * hasVariable is used to check whether to make reverseVariableMap
     */
    bool hasVariable;

public:

    NativeSmtConverter(const SMT_Info& smtInfo, MetaLevelSmtOpSymbol* extensionSymbol):
        smtInfo(smtInfo), extensionSymbol(extensionSymbol), hasVariable(false) {}

    virtual ~NativeSmtConverter(){
        smtManagerVariableMap.clear();
    }

    /*
     * Dag2Term should throw ExtensionException when any error occurs.
     *
     * makeExtensionVariable : User should check smtCheckerSymbol is null or not.
     * variableGenerator should know its dag parameter type before calling.
     */
    virtual T variableGenerator(DagNode* dag, ExprType exprType) = 0;
    virtual T makeVariable(VariableDagNode* v) = 0;
    
protected:

    DagNode * multipleGen(Vector<DagNode*>* dags, int i, MulType type){
        // Vector < DagNode* > arg(2);
        // arg[0] = (*dags)[i];
        // if (i == dags->length() - 1){
        //     return arg[0];
        // }
        // arg[1] = multipleGen(dags, i+1, type);
        // switch(type){
        //     case MulType::AND:
        //         return extensionSymbol->andBoolSymbol->makeDagNode(arg);
        //     case MulType::OR:
        //         return extensionSymbol->orBoolSymbol->makeDagNode(arg);
        //     case MulType::INT_ADD:
        //         return extensionSymbol->plusIntSymbol->makeDagNode(arg);
        //     case MulType::INT_SUB:
        //         return extensionSymbol->minusIntSymbol->makeDagNode(arg);
        //     case MulType::INT_MUL:
        //         return extensionSymbol->mulIntSymbol->makeDagNode(arg);
        //     case MulType::REAL_ADD:
        //         return extensionSymbol->plusRealSymbol->makeDagNode(arg);
        //     case MulType::REAL_SUB:
        //         return extensionSymbol->minusRealSymbol->makeDagNode(arg);
        //     case MulType::REAL_MUL:
        //         return extensionSymbol->mulRealSymbol->makeDagNode(arg);
        // }
        return nullptr;
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
