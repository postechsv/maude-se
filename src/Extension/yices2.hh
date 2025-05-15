#ifndef YICES2_EXTENSION_HH
#define YICES2_EXTENSION_HH

#include "smtInterface.hh"
#include "nativeSmt.hh"
#include "extGlobal.hh"
#include "yices.h"
#include <vector>
#include <gmpxx.h>

#include "simpleRootContainer.hh"


class YicesTerm : public SmtTerm
{
public:
    YicesTerm(term_t term, type_t type) : term(term), type(type) {};
    ~YicesTerm(){};

    term_t term;
    type_t type;
};

struct cmpExprById{
    bool operator( )(const YicesTerm &lhs, const YicesTerm &rhs) const {
        return lhs.term < rhs.term;
    }
};


class YicesConverter : public Converter, public NativeSmtConverter< YicesTerm*, cmpExprById >, private SimpleRootContainer
{
    NO_COPYING(YicesConverter);
public:
    YicesConverter(const SMT_Info &smtInfo, MetaLevelSmtOpSymbol* extensionSymbol);
	~YicesConverter(){};
    void prepareFor(VisibleModule* vmodule){};
    SmtTerm* dag2term(DagNode* dag){
        return dag2termInternal(dag);
    };
    DagNode* term2dag(SmtTerm* term){
        YicesTerm* t = dynamic_cast<YicesTerm*>(term);
        return term2dagInternal(t);
    }

public:
    // inline z3::context* getContext(){ return &ctx; };

private:
    // z3::context ctx;

private:
    // override
    YicesTerm* variableGenerator(DagNode *dag, ExprType exprType){ return nullptr; };
    YicesTerm* makeVariable(VariableDagNode* v){ return nullptr; };

    // Aux
    YicesTerm* dag2termInternal(DagNode* dag){ return nullptr; };
    DagNode* term2dagInternal(YicesTerm* term){ return nullptr; };

private:
    // Maude specific
    VisibleModule* vmodule;
    void markReachableNodes(){};
};



class YicesConnector : public Connector
{
public:
    YicesConnector(YicesConverter* conv);
	~YicesConnector(){};
    bool check_sat(std::vector<SmtTerm*> consts){ return false; };
    bool subsume(TermSubst* subst, SmtTerm* prev, SmtTerm* acc, SmtTerm* cur){ return false; };
    TermSubst* mk_subst(std::map<DagNode*, DagNode*>& subst_dict){ return nullptr; };
    SmtTerm* add_const(SmtTerm* acc, SmtTerm* cur){ return nullptr; };
    SmtModel* get_model(){ return nullptr; };
    void push(){};
    void pop(){};

    void print_model(){};
    void set_logic(const char* logic){};
    void reset(){};

    Converter* get_converter(){ return conv; };

private:
    // z3::solver *s;
    // z3::solver *s_v; // solver for validity check
    // z3::context ctx; // context for validity check

    // z3::expr translate(z3::expr& e);

    // int pushCount;
    YicesConverter* conv;
};

class YicesSmtManagerFactory : public SmtManagerFactory
{
public:
    Converter* createConverter(const SMT_Info& smtInfo, MetaLevelSmtOpSymbol* extensionSymbol){
        return new YicesConverter(smtInfo, extensionSymbol);
    }
    Connector* createConnector(Converter* conv){
        return new YicesConnector(dynamic_cast<YicesConverter*>(conv));
    }
};

class SmtManagerFactorySetter : public SmtManagerFactorySetterInterface
{
public:
    void set(){
        if (smtManagerFactory) delete smtManagerFactory;
        smtManagerFactory = new YicesSmtManagerFactory();
    };
};


// public:
//     YicesConverter(const SMT_Info &smtInfo);

//     // backward compatibility
//     Result assertDag(DagNode* dag);
//     Result checkDag(DagNode* dag);
//     VariableDagNode *makeFreshVariable(Term *baseVariable, const mpz_class &number);
//     SmtResult checkDagContextFree(DagNode *dag, ExtensionSymbol* extensionSymbol=nullptr);

//     // SMT manager requirements
//     DagNode *generateAssignment(DagNode *dagNode, SmtCheckerSymbol* smtCheckerSymbol);
//     DagNode *simplifyDag(DagNode *dagNode, ExtensionSymbol* extensionSymbol);
//     DagNode* applyTactic(DagNode* dagNode, DagNode* tacticTypeDagNode, ExtensionSymbol* extensionSymbol);

// private:

//     DagNode* GenerateDag(model_t *mdl, YicesTerm e, SmtCheckerSymbol* smtCheckerSymbol, ReverseSmtManagerVariableMap* rsv);
//     YicesTerm variableGenerator(DagNode *dag, ExprType exprType);

//     YicesTerm Dag2Term(DagNode *dag, ExtensionSymbol* extensionSymbol);
//     DagNode *Term2Dag(YicesTerm e, ExtensionSymbol* extensionSymbol, ReverseSmtManagerVariableMap* rsv=nullptr);

//     void setSolverTo(bool isLinear);
//     inline void setSimplificationStrategy(){
//         int32_t a = yices_context_enable_option(smtContext, "var-elim");
//         int32_t b = yices_context_enable_option(smtContext, "arith-elim");
//         // int32_t c = yices_context_enable_option(smtContext, "keep-ite");
//     }
//     bool isSolverLinear;
// };

#endif
