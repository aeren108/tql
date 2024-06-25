using namespace std;

#include <string.h>
#include "MyParser.h"
#include "FlexLexer.h"
#include "MyFlexLexer.h"

MyParser::MyParser(ofstream *os, ofstream *tos)
{
   lval=nullptr;
   base=nullptr;
   lexer=nullptr;
   tqlIR=new TQLIR();
   tqlIR->tqlMsgList=new TQLMsgList();

   this->os=os;
   this->tos = tos;
}

MyParser::~MyParser()
{
   delete tqlIR;
   delete lexer;
   delete base;
}

int MyParser::lex(yy::MyParserBase::semantic_type *const lval)
{
   this->lval=lval;
   return lexer->lex(lval);
}

void MyParser::parse(yy::MyParserBase *base, ifstream *is)
{
   this->base=base;
   lexer=new MyFlexLexer(this);
   lexer->switch_streams(is);
   base->parse();
}

int MyParser::getId()
{
   lval->STR=new string(lexer->YYText());
   return yy::MyParserBase::token::ID;
}

int MyParser::hexDigit(char c)
{
   if (c>='0' && c<='9')
      return c-'0';

   if (c>='A' && c<='F')
      return c-'A'+10;

   if (c>='a' && c<='f')
      return c-'a'+10;

   return -1;
}

string *MyParser::makeString(const char *rawStr)
{
   char *copyStr=strdup(rawStr),
        *p,
        *q;
   int   d;
   unsigned char c;

   q=copyStr;
   for (p=q+1;*p!='"';p++,q++)
   {
      switch (*p)
      {
         case '\\':
            p++;
            switch (*p)
            {
               case '\\':
                  *q='\\';
                  break;
               case 't':
                  *q='\t';
                  break;
               case 'r':
                  *q='\r';
                  break;
               case 'n':
                  *q='\n';
                  break;
               case '"':
                  *q='"';
                  break;
               case 'x':
                  p++;
                  c=(unsigned char)hexDigit(*p);
                  d=hexDigit(p[1]);
                  if (d>-1)
                  {
                     p++;
                     c=(c<<4) | (unsigned char)d;
                  }
                  *q=c;
            }
            break;
         default:
            *q=*p;

      }
   }
   *q=0;
   string *retVal=new string(copyStr);

   free(copyStr);

   return retVal;
}

int MyParser::getStr()
{
   lval->STR=makeString(lexer->YYText());
   return yy::MyParserBase::token::STR;
}

int MyParser::getNumber()
{
   lval->NUM=atof(lexer->YYText());
   return yy::MyParserBase::token::NUM;
}

int MyParser::getDPrefId()
{
   lval->DPREF=atoi(lexer->YYText()+1);
   return yy::MyParserBase::token::DPREF;
}

int MyParser::setTerminalSemantics(int semanticCode)
{
   lval->DPREF=semanticCode; // Takes advantage of union!
   return semanticCode;
}

void MyParser::setParseErrorLine()
{
   parseErrorLine=lexer->lineno();
}

int MyParser::getParseErrorLine()
{
   return parseErrorLine;
}

void MyParser::reportFindings()
{
   tqlIR->writeAsJSON(os);
   tqlIR->writeAsTXT(tos);
}

void MyParser::reportError()
{
   *os<<"Line "<<parseErrorLine<<": Syntax error.";
}

void yy::MyParserBase::error(const std::string &msg)
{
   driver->setParseErrorLine();
}

// Parser actions

TQLIdentifier *MyParser::createIdentifier(int dIndex, string *id)
{
   return new TQLIdentifier(dIndex, id);
}

TQLIdentifier *MyParser::createIdentifier(string *id)
{
   return new TQLIdentifier(id);
}

TQLExpNode *MyParser::createIdentifierNode(TQLIdentifier *id)
{
   return new TQLExpNode(this, id);
}

TQLExpNode *MyParser::createStrConstantNode(string *strConstant)
{
   return new TQLExpNode(this, strConstant);
}

TQLExpNode *MyParser::createNumberConstantNode(double numConstant)
{
   return new TQLExpNode(this, numConstant);
}

TQLExpNode *MyParser::createBoolConstantNode(bool boolConstant)
{
   return new TQLExpNode(this, boolConstant);
}

TQLExpNode *MyParser::createTableConstantNode(TQLExpNode *expNode)
{
   return new TQLExpNode(this, expNode);
}

TQLExpNode *MyParser::createCallNode(string *id, TQLExpNode *expNode)
{
   TQLExpNode *retVal=new TQLExpNode(this, OP_CALL);
   retVal->left=new TQLExpNode(this, new TQLIdentifier(id));
   retVal->right=expNode;

   retVal->left->parent = retVal;
   retVal->right->parent = retVal;

   return retVal;
}

TQLExpNode *MyParser::createUnaryOperatorNode(int opCode, TQLExpNode *expNode)
{
   TQLExpNode *retVal=new TQLExpNode(this, opCode);
   retVal->left=expNode;
   retVal->left->parent = retVal;

   return retVal;
}

TQLExpNode *MyParser::createExpressionNode(TQLExpNode *expressionNode, int opCode, TQLExpNode *commaOperandNode)
{
   TQLExpNode *retVal=new TQLExpNode(this, opCode);

   retVal->left=expressionNode;
   retVal->right=commaOperandNode;

   retVal->left->parent = retVal;
   retVal->right->parent = retVal;

   return retVal;
}

TQLExpNode *MyParser::createCommaOperandNode(TQLExpNode *commaOperandNode, int opCode, TQLExpNode *assignSideNode)
{
   TQLExpNode *retVal=new TQLExpNode(this, opCode);
   TQLExpNode *cursor,
              *parent=nullptr;

   for ( cursor=commaOperandNode;
         cursor!=nullptr && cursor->opCode==yy::MyParserBase::token::ASSIGN;
         cursor=cursor->right)
      parent=cursor;

   if (parent==nullptr)
   {
      retVal->left=commaOperandNode;
      retVal->right=assignSideNode;
   }
   else
   {
      parent->right=retVal;
      retVal->left=cursor;
      retVal->right=assignSideNode;
      retVal=commaOperandNode;
   }

   retVal->left->parent = retVal;
   retVal->right->parent = retVal;

   return retVal;
}

TQLExpNode *MyParser::createAssignSideNode(TQLExpNode *assignSideNode, int opCode, TQLExpNode *booleanOperandNode)
{
   TQLExpNode *retVal=new TQLExpNode(this, opCode);

   retVal->left=assignSideNode;
   retVal->right=booleanOperandNode;

   retVal->left->parent = retVal;
   retVal->right->parent = retVal;

   return retVal;
}

TQLExpNode *MyParser::createBooleanOperandNode(TQLExpNode *booleanOperandNode, int opCode, TQLExpNode *logicalOperandNode)
{
   TQLExpNode *retVal=new TQLExpNode(this, opCode);

   retVal->left=booleanOperandNode;
   retVal->right=logicalOperandNode;

   retVal->left->parent = retVal;
   retVal->right->parent = retVal;

   return retVal;
}

TQLExpNode *MyParser::createLogicalOperandNode(TQLExpNode *logicalOperandNode, int opCode, TQLExpNode *termNode)
{
   TQLExpNode *retVal=new TQLExpNode(this, opCode);

   retVal->left=logicalOperandNode;
   retVal->right=termNode;

   retVal->left->parent = retVal;
   retVal->right->parent = retVal;

   return retVal;
}

TQLExpNode *MyParser::createTermNode(TQLExpNode *termNode, int opCode, TQLExpNode *factorNode)
{
   TQLExpNode *retVal=new TQLExpNode(this, opCode);

   retVal->left=termNode;
   retVal->right=factorNode;

   retVal->left->parent = retVal;
   retVal->right->parent = retVal;

   return retVal;
}

TQLExpNode *MyParser::createUnaryOperandNode(TQLExpNode *unaryOperand, TQLExpNode *postfixExp)
{
   TQLExpNode *retVal;

   if (postfixExp!=nullptr)
   {
      postfixExp->left=unaryOperand;
      postfixExp->left->parent = postfixExp;
      retVal = postfixExp;
   }
   else
      retVal=unaryOperand;

   return retVal;
}

TQLVarDesc *MyParser::createFieldSpecifier(TQLTypeDesc *typeDesc, TQLIdentifier *id)
{
   return new TQLVarDesc(typeDesc, id);
}

TQLVarTable *MyParser::createFieldSpecifierList(TQLVarDesc *varDesc)
{
   TQLVarTable *retVal=new TQLVarTable();

   retVal->addVar(varDesc);

   return retVal;
}

TQLVarTable *MyParser::addToFieldSpecifierList(TQLVarTable *varTable, TQLVarDesc *varDesc)
{
   if (!varTable->addVar(varDesc))
   {
      tqlIR->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
            2, getLine(), new string("Multiple declaration of specifier.")));
   }

   return varTable;
}

TQLExpNode *MyParser::createPostfixOpNode(TQLVarTable *varTable)
{
   TQLExpNode *retVal=new TQLExpNode(this, OP_MUT);

   TQLTypeDesc *td=new TQLTypeDesc(TQLType::typeObject);
   td->members=varTable;

   retVal->typeDesc=td;

   return retVal;
}

TQLTypeDesc *MyParser::createTypeDescriptor(TQLType t)
{
   return new TQLTypeDesc(t);
}

TQLEvaStatement *MyParser::createEvaluationStat(TQLExpNode *exp)
{
   return new TQLEvaStatement(this, exp);
}

TQLLetAssignment *MyParser::createLetAssignment(string *id, TQLExpNode *exp)
{
   return new TQLLetAssignment(id, exp);
}

TQLLetStatement *MyParser::createLetStatement(TQLLetAssignment *letAssignment)
{
   return new TQLLetStatement(this, letAssignment);
}

TQLLetStatement *MyParser::appendToLetStatement(TQLLetStatement *letStatement, TQLLetAssignment *letAssignment)
{
   letStatement->appendLetAssignment(letAssignment);

   return letStatement;
}

TQLAppendStatement *MyParser::createAppendStat(TQLExpNode *exp)
{
   return new TQLAppendStatement(this, exp);
}

TQLCompoundStatement *MyParser::createEmptyCompoundStatement()
{
   return new TQLCompoundStatement(this);
}

TQLCompoundStatement *MyParser::createCompoundStat(TQLStatement *statement)
{
   return new TQLCompoundStatement(this, statement);
}

TQLCompoundStatement *MyParser::appendToCompoundStat(TQLCompoundStatement *compoundStatement, TQLStatement *statement)
{
   compoundStatement->appendStatement(statement);

   return compoundStatement;
}

TQLWhileStatement *MyParser::createWhileStatement(TQLExpNode *expNode, TQLStatement *loopStatement)
{
   return new TQLWhileStatement(this, expNode, loopStatement);
}

TQLIfStatement *MyParser::createIfStatement(TQLExpNode *expNode, TQLStatement *trueStatement, TQLStatement *falseStatement)
{
   return new TQLIfStatement(this, expNode, trueStatement, falseStatement);
}

TQLCompoundStatement *MyParser::setTQL(TQLCompoundStatement *tqlStat)
{
   tqlIR->tqlStat=tqlStat;
   return tqlStat;
}

void MyParser::generateIC()
{
   tqlIR->captureVariables();
   tqlIR->computeAddresses();
   tqlIR->computeExpAddr();
   tqlIR->emitIC();
}

int MyParser::getLine()
{
   return lexer->lineno();
}

TQLIR* MyParser::getTQLIR()
{
   return tqlIR;
}