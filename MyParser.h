/*
 * MyParser.h
 *
 *  Created on: Mar 24, 2024
 *      Author: erkan
 */

#ifndef MYPARSER_H_
#define MYPARSER_H_

#include "tqlparse.tab.hh"
#include <iostream>
#include <fstream>
#include "tqlir.h"

namespace yy
{
   class MyParserBase;
};

class MyFlexLexer;

class MyParser
{
   yy::MyParserBase    *base;
   MyFlexLexer         *lexer;
   ofstream            *os, *tos;

   TQLIR               *tqlIR;

   // Variable to integrate token semantics
   yy::MyParserBase::semantic_type *lval;

   int              parseErrorLine=-1;

   string *makeString(const char *rawStr);
   int hexDigit(char c);
public:
   MyParser(ofstream *os, ofstream *tos);
   ~MyParser();

   void parse(yy::MyParserBase *base, ifstream *is);
   int lex(yy::MyParserBase::value_type *lval);

   int getId();
   int getStr();
   int getNumber();
   int getDPrefId();
   int setTerminalSemantics(int semanticCode);

   void setParseErrorLine();
   int getParseErrorLine();

   void reportFindings();
   void reportError();

   // Parser actions

   TQLIdentifier *createIdentifier(int dIndex, string *sym);
   TQLIdentifier *createIdentifier(string *sym);

   TQLExpNode *createIdentifierNode(TQLIdentifier *id);
   TQLExpNode *createNumberConstantNode(double num);
   TQLExpNode *createBoolConstantNode(bool bVal);
   TQLExpNode *createStrConstantNode(string *str);
   TQLExpNode *createCallNode(string *id, TQLExpNode *expNode);
   TQLExpNode *createTableConstantNode(TQLExpNode *expNode);

   TQLExpNode *createUnaryOperatorNode(int opCode, TQLExpNode *expNode);
   TQLExpNode *createUnaryOperandNode(TQLExpNode *unaryOperand, TQLExpNode *postfixExp);
   TQLExpNode *createPostfixOpNode(TQLVarTable *varTable);
   TQLExpNode *createTermNode(TQLExpNode *termNode, int opCode, TQLExpNode *factorNode);
   TQLExpNode *createLogicalOperandNode(TQLExpNode *logicalOperandNode, int opCode, TQLExpNode *termNode);
   TQLExpNode *createBooleanOperandNode(TQLExpNode *booleanOperandNode, int opCode, TQLExpNode *logicalOperandNode);
   TQLExpNode *createAssignSideNode(TQLExpNode *assignSideNode, int opCode, TQLExpNode *booleanOperandNode);
   TQLExpNode *createCommaOperandNode(TQLExpNode *commaOperandNode, int opCode, TQLExpNode *assignSideNode);
   TQLExpNode *createExpressionNode(TQLExpNode *expressionNode, int opCode, TQLExpNode *commaOperandNode);

   TQLVarDesc *createFieldSpecifier(TQLTypeDesc *typeDesc, TQLIdentifier *id);
   TQLVarTable *createFieldSpecifierList(TQLVarDesc *varDesc);
   TQLVarTable *addToFieldSpecifierList(TQLVarTable *varTable, TQLVarDesc *varDesc);
   TQLTypeDesc *createTypeDescriptor(TQLType t);

   TQLEvaStatement *createEvaluationStat(TQLExpNode *exp);

   TQLLetAssignment *createLetAssignment(string *id, TQLExpNode *exp);
   TQLLetStatement *createLetStatement(TQLLetAssignment *letAssignment);
   TQLLetStatement *appendToLetStatement(TQLLetStatement *letStatement, TQLLetAssignment *letAssignment);

   TQLAppendStatement *createAppendStat(TQLExpNode *exp);

   TQLCompoundStatement *createEmptyCompoundStatement();
   TQLCompoundStatement *createCompoundStat(TQLStatement *statement);
   TQLCompoundStatement *appendToCompoundStat(TQLCompoundStatement *compoundStatement, TQLStatement *statement);

   TQLWhileStatement *createWhileStatement(TQLExpNode *expNode, TQLStatement *loopStatement);
   TQLIfStatement *createIfStatement(TQLExpNode *expNode, TQLStatement *trueStatement, TQLStatement *falseStatement);

   TQLCompoundStatement *setTQL(TQLCompoundStatement *tqlStat);
   void generateIC();

   int getLine();
   TQLIR *getTQLIR();
};


#endif /* MYPARSER_H_ */
