#ifndef _IR_H
#define _IR_H

#include <vector>
#include <string>

#define PREC_COUNT 6

enum class TQLType
{
   typeNA=-1,
   typeNumber=1,
   typeBool=2,
   typeString=3,
   typeObject=4
};

class MyParser;
class TQLVarDesc;
class TQLTypeDesc;
class TQLIdentifier;
class TQLIC;

class TQLVarList
{

   protected:
      vector<TQLVarDesc*> *vars=nullptr;

   public:

      TQLVarList();
      virtual ~TQLVarList();

      bool addVar(TQLVarDesc *item);
      TQLVarDesc *findVar(TQLIdentifier *id);
      TQLVarDesc *findVarStripped(TQLIdentifier *id);
      TQLVarDesc *varAt(int ndx);
      int count();

      TQLVarDesc *operator[] (int ndx);

      virtual void writeAsJSON(ofstream *outStream);

};

class TQLTypeDesc
{
   private:
      int refCount=0;

   public :
      TQLType                 type;
      TQLVarList             *members=nullptr;

      TQLTypeDesc(TQLType type);
      virtual ~TQLTypeDesc();

      const char *basicTypeStr();
      static const char *basicTypeStr(TQLType type);

      bool addMember(TQLVarDesc *member);
      bool equals(TQLTypeDesc *other);

      TQLTypeDesc *xProd(TQLTypeDesc *other);
      TQLTypeDesc *addRef();

      static void removeRef(TQLTypeDesc *td);
      virtual void writeAsJSON(ofstream *outStream);
};

class TQLVarTable : public TQLVarList
{
   public:
      TQLVarTable();
      virtual ~TQLVarTable();
};

class TQLVarDesc
{
   public:
      TQLTypeDesc   *varType;
      TQLIdentifier *id;
      int            address;

      TQLVarDesc(TQLTypeDesc *varType, TQLIdentifier *id);
      virtual ~TQLVarDesc();

      bool isTypeEqual(TQLVarDesc *other);

      virtual void writeAsJSON(ofstream *outStream);
};


class TQLIdentifier
{
   public:
      int dIndex;
      string *id;

      TQLIdentifier(string *id);
      TQLIdentifier(int dIndex, string *id);
      bool equals(string *id);
      bool equals(TQLIdentifier *other);
      virtual ~TQLIdentifier();

      virtual void writeAsJSON(ofstream *outStream);
};

class TQLNode
{
   public:
      TQLNode *parent = nullptr;
      MyParser *driver;
      int lineno = -1;
      int addrOffset = -1;
      int stackSize = 0;

      TQLNode(MyParser *driver);
      virtual ~TQLNode();

      virtual TQLVarDesc *resolveSymbol(TQLIdentifier* id);
      virtual void computeExpAddr()=0;
      virtual void computeAddresses()=0;
      virtual void captureVariables()=0;
      virtual void emitIC(TQLIC *ic)=0;
      virtual void writeAsJSON(ofstream *outStream)=0;
};

class TQLStatement : public TQLNode
{
   public:
      TQLVarTable  *vars;

      TQLStatement(MyParser *driver);
      ~TQLStatement();
};

#define OP_CONST  3000
#define OP_ID     3001
#define OP_CALL   3002
#define OP_MUT    3003
#define OP_JMP    3010
#define OP_JT     3011
#define OP_JF     3022
#define OP_SETV   3023
#define OP_ADDSP  3024
#define OP_PUSH   3025
#define OP_POP    3026
#define OP_FID    3027

class TQLExpNode : public TQLNode
{
   public:
      int            opCode,
                     address=-1;
      TQLExpNode    *left=nullptr,
                    *right=nullptr;
      TQLTypeDesc   *typeDesc=nullptr;
      TQLVarTable   *vars=nullptr;

      union
      {
         string        *strConstant=nullptr;
         double         numConstant;
         bool           boolConstant;
         TQLIdentifier *id;
      };

      TQLExpNode(MyParser *driver, int opCode);
      TQLExpNode(MyParser *driver, string *strConstant);
      TQLExpNode(MyParser *driver, double numConstant);
      TQLExpNode(MyParser *driver, bool boolConstant);
      TQLExpNode(MyParser *driver, TQLExpNode *tableNameExp);
      TQLExpNode(MyParser *driver, TQLIdentifier *id);
      virtual ~TQLExpNode();

      virtual void writeAsJSON(ofstream *outStream);
      void writeStrValue(ofstream *os);

      const char *opStr();

      TQLVarDesc *resolveSymbol(TQLIdentifier *id);
      void computeExpAddr();
      void computeAddresses();
      void captureVariables();
      void emitIC(TQLIC *ic);
};

class TQLIfStatement : public TQLStatement
{
   public:
      TQLStatement  *trueStat,
                    *falseStat;
      TQLExpNode    *exp;

      TQLIfStatement(MyParser *driver, TQLExpNode *exp, TQLStatement *trueStat);
      TQLIfStatement(MyParser *driver, TQLExpNode *exp, TQLStatement *trueStat, TQLStatement *falseStat);

      ~TQLIfStatement();

      virtual void writeAsJSON(ofstream *outStream);

      void computeExpAddr();
      void computeAddresses();
      void captureVariables();
      void emitIC(TQLIC *ic);
};

class TQLWhileStatement : public TQLStatement
{
   public:
      TQLStatement  *loopStat;
      TQLExpNode    *exp;

      TQLWhileStatement(MyParser *driver, TQLExpNode *exp, TQLStatement *loopStat);
      ~TQLWhileStatement();

      virtual void writeAsJSON(ofstream *outStream);

      void computeExpAddr();
      void computeAddresses();
      void captureVariables();
      void emitIC(TQLIC *ic);
};

class TQLAppendStatement : public TQLStatement
{
   public:
      TQLExpNode *exp;

      TQLAppendStatement(MyParser *driver, TQLExpNode *exp);
      virtual ~TQLAppendStatement();

      virtual void writeAsJSON(ofstream *outStream);

      void computeExpAddr();
      void computeAddresses();
      void captureVariables();
      void emitIC(TQLIC *ic);
};

class TQLLetAssignment
{
   public:
      string     *id;
      TQLExpNode *exp;
      

      TQLLetAssignment(string *id, TQLExpNode *exp)
         : id(id), exp(exp)
      {
      }
};

class TQLLetStatement : public TQLStatement
{
   public:
      vector<TQLLetAssignment *> *letAssignments;

      TQLLetStatement(MyParser *driver, TQLLetAssignment *letAssignment);
      virtual ~TQLLetStatement();

      void appendLetAssignment(TQLLetAssignment *letAssignment);

      virtual void writeAsJSON(ofstream *outStream);

      void computeExpAddr();
      void computeAddresses();
      void captureVariables();
      void emitIC(TQLIC *ic);
};

class TQLEvaStatement : public TQLStatement
{
   public:
      TQLExpNode    *exp;
      TQLEvaStatement(MyParser *driver, TQLExpNode *exp);
      virtual ~TQLEvaStatement();

      virtual void writeAsJSON(ofstream *outStream);

      void computeExpAddr();
      void computeAddresses();
      void captureVariables();
      void emitIC(TQLIC *ic);
};

class TQLCompoundStatement : public TQLStatement
{
   public :
      vector<TQLStatement *> *stats;
      TQLVarTable *varTable, *prevTable;

      TQLCompoundStatement(MyParser *driver);
      TQLCompoundStatement(MyParser *driver, TQLStatement *statement);
      ~TQLCompoundStatement();

      void appendStatement(TQLStatement *statement);
      virtual void writeAsJSON(ofstream *outStream);

      TQLVarDesc *resolveSymbol(TQLIdentifier *id) override;

      void computeExpAddr();
      void computeAddresses();
      void captureVariables();
      void emitIC(TQLIC *ic);
};

enum class TQLMsgType
{
   error=1,
   warning=2,
   info=3
};

class TQLFuncDesc : public TQLVarDesc
{
   public:
      TQLVarList *parameters;
      TQLTypeDesc *retType;
      virtual ~TQLFuncDesc();

      TQLFuncDesc(TQLIdentifier *id, TQLTypeDesc *retType);

      bool addParameter(string *id, TQLTypeDesc *paraType);
      TQLVarDesc *getParameter(int ndx);
};

class TQLFuncTable : TQLVarList
{
   public:
      TQLFuncTable();
      virtual ~TQLFuncTable();

      bool addFunction(TQLFuncDesc *func);
      TQLFuncDesc *findFunc(string *id);
      int findIndex(string *id);
      void preset();
};

#define ERR_DEFWITHINTRINSICNAME    1

class TQLMsg
{
   public:
      string     *msg;
      TQLMsgType  msgType;
      int         line,
                  msgCode;

      TQLMsg(TQLMsgType msgType, int msgCode, int line, string *msg);
      virtual ~TQLMsg();

      virtual void writeAsJSON(ofstream *outStream);
};

class TQLMsgList
{
   private:
      vector<TQLMsg *>  *msgList;
   public:
      TQLMsgList();
      virtual ~TQLMsgList();

      void add(TQLMsg *error);
      virtual void writeAsJSON(ofstream *outStream);
};

class TQLICInst
{
   public:
      int      opCode;
      int      p1;
      TQLType  type;

      union
      {
         string        *strConstant=nullptr;
         double         numConstant;
         bool           boolConstant;
         TQLIdentifier *id;
      };

      TQLICInst(int opCode, int p1);
      TQLICInst(int opCode, int p1, TQLType type);
      virtual ~TQLICInst();

      const char *opStr();
      const char *typeStr();
      string constStr();

      virtual void writeAsJSON(ofstream *outStream);
      virtual void writeAsTXT(int instno, ofstream *outsream);
};

class TQLIC
{
   public:
      vector<TQLICInst *>   *code;

      TQLIC();
      virtual ~TQLIC();

      int count();
      void addInstruction(TQLICInst *inst);
      TQLICInst *instructionAt(int loc);
      virtual void writeAsJSON(ofstream *outStream);
      virtual void writeAsTXT(ofstream *outStream);
};

class TQLIR
{
   public:
      TQLCompoundStatement   *tqlStat=nullptr;
      TQLIC                  *tqlIC=nullptr;
      TQLMsgList             *tqlMsgList=nullptr;
      TQLFuncTable           *funcTable=nullptr;

      TQLIR();
      virtual ~TQLIR();

      void computeExpAddr();
      void computeAddresses();
      void captureVariables();
      void emitIC();

      virtual void writeAsJSON(ofstream *outStream);
      virtual void writeAsTXT(ofstream *outStream);
};

class TQLTable
{
   public:
      string *tableName;
      TQLVarTable *fields;

      TQLTable(string *tableName, TQLVarTable *fields);
      ~TQLTable();
};

#endif
