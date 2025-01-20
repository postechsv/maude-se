/*

    This file is part of the Maude 2 interpreter.

    Copyright 1997-2017 SRI International, Menlo Park, CA 94025, USA.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

*/

//
//      Class for generating SMT variables, version for CVC4 support.
//
#ifndef _z3_EngineWrapper_hh_
#define _z3_EngineWrapper_hh_
#include "smtEngineWrapperEx.hh"
#include "z3++.h"
#include <vector>
#include <sstream>
#include <gmpxx.h>

using namespace z3;

struct cmpExprById{
    bool operator( )(const expr &lhs, const expr &rhs) const {
        return lhs.id() < rhs.id();
    }
};

class VariableGenerator : public SmtEngineWrapperEx<expr, cmpExprById> {
    NO_COPYING(VariableGenerator);
public:
    VariableGenerator(const SMT_Info &smtInfo);
    ~VariableGenerator();


    /*
     * SMT_Engine wrapper interface implementation
     */
    Result assertDag(DagNode *dag);
    Result checkDag(DagNode *dag);
    Result checkDagContextFree(DagNode *dag, ExtensionSymbol* extensionSymbol);
    VariableDagNode *makeFreshVariable(Term *baseVariable, const mpz_class &number);
    void clearAssertions();
    void push();
    void pop();

public:

    virtual Connector* getConnector() = 0;
    virtual Converter* getConverter() = 0;


    /*
     * SmtManager class interface implementation
     */
    expr Dag2Term(DagNode *dag, ExtensionSymbol* extensionSymbol);
    DagNode* Term2Dag(expr e, ExtensionSymbol* extensionSymbol, ReverseSmtManagerVariableMap* rsv = nullptr);
    DagNode* generateAssignment(DagNode *dagNode, SmtCheckerSymbol* extensionSymbol);
    DagNode* simplifyDag(DagNode *dagNode, ExtensionSymbol* extensionSymbol);
    DagNode* applyTactic(DagNode* dagNode, DagNode* tacticTypeDagNode, ExtensionSymbol* extensionSymbol);
    expr variableGenerator(DagNode *dag, ExprType exprType);


private:
    context ctx;
    solver *s;

    int pushCount;

    DagNode* InferTerm2Dag(expr e, DagNode* dagNode, ExtensionSymbol* extensionSymbol);
    tactic Dag2Tactic(TacticApplySymbol* tacticApplySymbol, DagNode* dagNode);

    DagNode* GenerateDag(expr lhs, expr rhs, SmtCheckerSymbol* smtCheckerSymbol, ReverseSmtManagerVariableMap* rsv);
};


#endif
