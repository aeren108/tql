%language "c++"
%define api.parser.class {MyParserBase}

%{
using namespace std;
#include <iostream>
#include <fstream>
%}
%code requires
{
#include <vector>
#include "tqlir.h"
class MyParser;
};

%code
{
   #include "MyParser.h"
   #define yylex(x) driver->lex(x)
}

%define api.value.type union

%parse-param {MyParser *driver}
%start tql

%token SOB
%token EOB
%token OB
%token CB
%token <int> PLUS
%token <int> MINUS
%token <int> MUL
%token <int> DIV
%token LP
%token RP
%token <int> ASSIGN
%token <int> LT
%token <int> LTE
%token <int> GT
%token <int> GTE
%token <int> EQ
%token <int> NEQ
%token <int> MAT
%token <int> BAND
%token <int> BOR
%token IF
%token ELSE
%token APPEND
%token WHILE
%token LET
%token AT
%token DOT
%token <string *> ID
%token <string *> STR
%token <double> NUM
%token <int> COMMA
%token <int> NOT
%token SEMICOLON
%token STRINGTYPE
%token NUMBERTYPE
%token BOOLTYPE
%token TRUE
%token FALSE
%token <int> DPREF

%nterm <int> dPrefix
%nterm <TQLIdentifier *> identifier
%nterm <TQLExpNode *> expression
%nterm <TQLExpNode *> commaOperand
%nterm <TQLExpNode *> assignSide
%nterm <TQLExpNode *> booleanOperand
%nterm <TQLExpNode *> logicalOperand
%nterm <TQLExpNode *> term
%nterm <TQLExpNode *> factor
%nterm <TQLExpNode *> unaryOperand
%nterm <int> unaryOperator
%nterm <TQLExpNode *> unaryOperandBase
%nterm <TQLExpNode *> postfixOperator
%nterm <TQLVarDesc *> fieldSpecifier
%nterm <TQLTypeDesc *> type
%nterm <TQLVarTable *> fieldSpecifierList
%nterm <int> factorOp
%nterm <int> termOp
%nterm <int> logicalOp
%nterm <int> booleanOp
%nterm <TQLEvaStatement *> evaluationStatement
%nterm <TQLLetStatement *> varDefList
%nterm <TQLLetAssignment *> varDef
%nterm <TQLLetStatement *> letStatement
%nterm <TQLAppendStatement *> appendStatement
%nterm <TQLCompoundStatement *> compoundStatement
%nterm <TQLCompoundStatement *> statementList
%nterm <TQLStatement *> statement
%nterm <TQLWhileStatement *> whileStatement
%nterm <TQLStatement *> elsePart
%nterm <TQLIfStatement *> ifStatement
%nterm <TQLCompoundStatement *> tql

%left ELSE

%%
   
tql: statementList {$$=driver->setTQL($1);}
   | {$$=driver->setTQL(nullptr);};

statementList
		: statement {$$=driver->createCompoundStat($1);}
		| statementList statement {$$=driver->appendToCompoundStat($1, $2);};

statement
		: ifStatement {$$=$1;}
      | whileStatement {$$=$1;}
      | letStatement {$$=$1;}
      | appendStatement {$$=$1;}
      | evaluationStatement {$$=$1;}
      | compoundStatement {$$=$1;};
       
ifStatement
		: IF LP expression RP statement elsePart {$$=driver->createIfStatement($3, $5, $6);};
elsePart
 		: ELSE statement {$$=$2;}
 		| {$$=nullptr;};
 		
whileStatement
		: WHILE LP expression RP statement {$$=driver->createWhileStatement($3, $5);};
 		
compoundStatement
		: SOB statementList EOB {$$=$2;}
		| SOB EOB {$$=driver->createEmptyCompoundStatement();};

letStatement:
		LET varDefList SEMICOLON {$$=$2;};
		
varDefList
      : varDef {$$=driver->createLetStatement($1);}
      | varDefList COMMA varDef {$$=driver->appendToLetStatement($1, $3);};
       
varDef  : ID ASSIGN assignSide {$$=driver->createLetAssignment($1, $3);};
		
appendStatement
		: APPEND expression SEMICOLON {$$=driver->createAppendStat($2);};
		
evaluationStatement
		: expression SEMICOLON {$$=driver->createEvaluationStat($1);};
		
type    
		: STRINGTYPE {$$=driver->createTypeDescriptor(TQLType::typeString);}
		| NUMBERTYPE {$$=driver->createTypeDescriptor(TQLType::typeNumber);}
		| BOOLTYPE {$$=driver->createTypeDescriptor(TQLType::typeBool);};
		
expression
      : commaOperand {$$=$1;}
      | expression COMMA commaOperand {$$=driver->createExpressionNode($1, $2, $3);};
		
commaOperand
		: assignSide {$$=$1;}
		| commaOperand ASSIGN assignSide {$$=driver->createCommaOperandNode($1, $2, $3);};
		
assignSide
      : booleanOperand {$$=$1;}
      | assignSide booleanOp booleanOperand {$$=driver->createAssignSideNode($1, $2, $3);};
     
booleanOp
      : BAND {$$=$1;}
      | BOR {$$=$1;};
		
booleanOperand
      : logicalOperand {$$=$1;}
      | booleanOperand logicalOp logicalOperand {$$=driver->createBooleanOperandNode($1, $2, $3);};
     
logicalOp
      : EQ {$$=$1;}
      | NEQ {$$=$1;}
      | LT {$$=$1;}
      | LTE {$$=$1;}
      | GT {$$=$1;}
      | GTE {$$=$1;};
     
logicalOperand
      : term {$$=$1;}
      | logicalOperand termOp term {$$=driver->createLogicalOperandNode($1, $2, $3);};
     
termOp
      : PLUS {$$=$1;}
      | MINUS {$$=$1;};
     
term  
      : factor {$$=$1;}
      | term factorOp factor {$$=driver->createTermNode($1, $2, $3);};
     
factorOp
      : MUL {$$=$1;}
      | DIV {$$=$1;}
		| MAT {$$=$1;};
		
factor
		: unaryOperator unaryOperand {$$=driver->createUnaryOperatorNode($1, $2);}
		| unaryOperand {$$=$1;};
		
unaryOperator
		: MINUS {$$=$1;}
		| NOT {$$=$1;};
		
unaryOperand
	    : unaryOperandBase postfixOperator {$$=driver->createUnaryOperandNode($1,$2);};
		
unaryOperandBase
		: identifier {$$=driver->createIdentifierNode($1);}
		| ID LP expression RP {$$=driver->createCallNode($1, $3);}
      | ID LP RP {$$=driver->createCallNode($1, nullptr);}
		| NUM   {$$=driver->createNumberConstantNode($1);}
		| TRUE  {$$=driver->createBoolConstantNode(true);}
		| FALSE {$$=driver->createBoolConstantNode(false);}
		| STR   {$$=driver->createStrConstantNode($1);}
		| LP expression RP {$$=$2;}
		| AT OB expression CB {$$=driver->createTableConstantNode($3);};
		
		
postfixOperator
	   : OB fieldSpecifierList CB {$$=driver->createPostfixOpNode($2);}
	   | {$$=nullptr;};
	   
fieldSpecifierList
		: fieldSpecifier {$$=driver->createFieldSpecifierList($1);}
		| fieldSpecifierList COMMA fieldSpecifier {$$=driver->addToFieldSpecifierList($1, $3);};
		
fieldSpecifier
		: type identifier {$$=driver->createFieldSpecifier($1, $2);};
		
identifier
	   : dPrefix ID {$$=driver->createIdentifier($1, $2);}
	   | ID {$$=driver->createIdentifier($1);};
	   
dPrefix
      : DPREF DOT {$$=$1;};
%%