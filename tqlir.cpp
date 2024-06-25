using namespace std;

#include "MyParser.h"
#include <iomanip>
#include <string>

TQLVarTable *curTable = nullptr;

// Type descriptors and type signatures

TQLTypeDesc::TQLTypeDesc(TQLType type)
      :type(type)
{

}

bool TQLTypeDesc::addMember(TQLVarDesc *memDesc)
{
   if (members==nullptr)
      members=new TQLVarList();

   return members->addVar(memDesc);
}

bool TQLTypeDesc::equals(TQLTypeDesc *other)
{
   if (type!=other->type)
      return false;

   if (members!=nullptr && other->members!=nullptr)
   {
      int c=members->count();

      if (c==other->members->count())
      {
         if (c>0)
         {
            for (int i=0;i<c;i++)
               if (!(*members)[i]->isTypeEqual((*members)[i]))
                  return false;
         }
      }
      else
         return false;
   }
   else
   {
      if (members!=nullptr || other->members!=nullptr)
         return false;
   }

   return true;

}

TQLTypeDesc::~TQLTypeDesc()
{
   if (members!=nullptr)
      delete members;
}

const char *TQLTypeDesc::basicTypeStr(TQLType type)

{
   switch (type)
   {
      case TQLType::typeBool:
         return "boolean";
      case TQLType::typeNumber:
         return "number";
      case TQLType::typeString:
         return "string";
      case TQLType::typeObject:
         return "object";
      default: ;

   }

   return "";

}

const char *TQLTypeDesc::basicTypeStr()
{
  return basicTypeStr(type);
}

void TQLTypeDesc::writeAsJSON(ofstream *outStream)
{

   (*outStream) << "{\"type\": \"";
   (*outStream) << basicTypeStr();
   (*outStream) << "\"";

   if (type==TQLType::typeObject)
   {
      (*outStream) << ", \"members\" : ";
      
      if (members!=nullptr)
         members->writeAsJSON(outStream);
      else
         (*outStream) << "null";
   }

   (*outStream) << "}";
}

TQLTypeDesc *TQLTypeDesc::addRef()
{
   refCount++;
   return this;
}

void TQLTypeDesc::removeRef(TQLTypeDesc *td)
{
   td->refCount--;
   if (td->refCount==0)
      delete td;
}


// TQLVarDesc
TQLVarDesc::TQLVarDesc(TQLTypeDesc *varType, TQLIdentifier *id)
   :varType(varType), id(id)
{
   varType->addRef();
   address=-1;
}

TQLVarDesc::~TQLVarDesc()
{
   delete id;
   TQLTypeDesc::removeRef(varType);
}

bool TQLVarDesc::isTypeEqual(TQLVarDesc *other)
{
   return varType->equals(other->varType);
}

void TQLVarDesc::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "{\"id\": \"";
   id->writeAsJSON(outStream);

   (*outStream) << "\", \"address\": " << address << "";

   (*outStream) << ", \"type\":";
   varType->writeAsJSON(outStream);
   (*outStream) << "}";
}

//TQLIdentifier class
TQLIdentifier::TQLIdentifier(string *id)
   :dIndex(-1), id(id)
{
}

TQLIdentifier::TQLIdentifier(int dIndex, string *id)
   :dIndex(dIndex), id(id)
{
}

bool TQLIdentifier::equals(string *id)
{
   if (dIndex!=-1)
      return false;

   return this->id->compare(*id)==0;
}

bool TQLIdentifier::equals(TQLIdentifier *other)
{
   if (dIndex!=other->dIndex)
      return false;

   return id->compare(*other->id)==0;
}

TQLIdentifier::~TQLIdentifier()
{
   delete id;
}

void TQLIdentifier::writeAsJSON(ofstream *outStream)
{
   if (dIndex>0)
      (*outStream) << "$" << dIndex << ".";

   (*outStream) << *id;
}

// Expression node class
TQLExpNode::TQLExpNode(MyParser *driver, int opCode) : TQLNode(driver),
      opCode(opCode)
{

}

TQLExpNode::TQLExpNode(MyParser *driver, TQLIdentifier *id) : TQLNode(driver),
      opCode(OP_ID), id(id)
{
}

TQLExpNode::TQLExpNode(MyParser *driver, string *strConstant) : TQLNode(driver),
   strConstant(strConstant), opCode(OP_CONST), typeDesc(new TQLTypeDesc(TQLType::typeString))
{
}

TQLExpNode::TQLExpNode(MyParser *driver, double numConstant) : TQLNode(driver),
   numConstant(numConstant), opCode(OP_CONST), typeDesc(new TQLTypeDesc(TQLType::typeNumber))
{
}

TQLExpNode::TQLExpNode(MyParser *driver, bool boolConstant) : TQLNode(driver),
   boolConstant(boolConstant), opCode(OP_CONST), typeDesc(new TQLTypeDesc(TQLType::typeBool))
{
}

TQLExpNode::TQLExpNode(MyParser *driver, TQLExpNode *tableNameExp) : TQLNode(driver),
   boolConstant(boolConstant), opCode(OP_CONST), typeDesc(new TQLTypeDesc(TQLType::typeObject))
{
   left=tableNameExp;
   left->parent = this;
}

void TQLExpNode::writeStrValue(ofstream *os)
{
   static  unsigned char const *hd=(unsigned char const *)"0123456789abcdef";
   string  t=string();
   int     l=(int)strConstant->length();

   t+='"';
   for (int i=0;i<l;i++)
   {
      char c=(*strConstant)[i];

      switch (c)
      {
         case '\n':
            t+="\\n";
            break;
         case '\t':
            t+="\\t";
            break;
         case '\r':
            t+="\\r";
            break;
         case '\\':
            t+="\\\\";
            break;
         default:
            if (c<32)
            {
               t+="\\x";
               t+=hd[c>>4];
               t+=hd[c&0xf];
            }
            else
               t+=(*strConstant)[i];
      }
   }

   t+='"';
   (*os) << t;
}

const char *TQLExpNode::opStr()
{
   switch (opCode)
   {
      case OP_CONST:
         return "const";
      case OP_ID:
         return "id";
      case OP_CALL:
         return "call()";
      case OP_MUT:
         return "mutation[]";
      case yy::MyParserBase::token::ASSIGN:
         return "=";
      case yy::MyParserBase::token::PLUS:
         return "+";
      case yy::MyParserBase::token::MINUS:
         return "-";
      case yy::MyParserBase::token::DIV:
         return "/";
      case yy::MyParserBase::token::MUL:
         return "*";
      case yy::MyParserBase::token::EQ:
         return "==";
      case yy::MyParserBase::token::NEQ:
         return "!=";
      case yy::MyParserBase::token::LT:
         return "<";
      case yy::MyParserBase::token::LTE:
         return "<=";
      case yy::MyParserBase::token::GT:
         return ">";
      case yy::MyParserBase::token::GTE:
         return ">=";
      case yy::MyParserBase::token::BOR:
         return "||";
      case yy::MyParserBase::token::BAND:
         return "&&";
      case yy::MyParserBase::token::COMMA:
         return ",";
      case yy::MyParserBase::token::MAT:
         return "->";
   }

   return "unknown";
}

void TQLExpNode::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "{\"op\": \"" << opStr() << "\", \"opCode\":\"" << opCode << "\"";
   if (typeDesc!=nullptr)
   {
      (*outStream) << ", \"type\": ";
      typeDesc->writeAsJSON(outStream);
   }

   (*outStream) << ", \"lineno\": " << lineno;
   (*outStream) << ", \"address\": " << address;
   (*outStream) << ", \"this\": \"" << this << "\"";
   (*outStream) << ", \"parent\": \"" << parent << "\"";
   
   switch (opCode)
   {
      case OP_CONST:
         switch (typeDesc->type)
         {
            case TQLType::typeBool:
               (*outStream) << ", \"value\": " << boolConstant;
               break;
            case TQLType::typeNumber:
               (*outStream) << ", \"value\": " << numConstant;
               break;
            case TQLType::typeString:
               (*outStream) << ", \"value\": ";
               writeStrValue(outStream);
               break;
         }
         break;
      case OP_ID:
         (*outStream)<<", \"id\":\"";
         id->writeAsJSON(outStream);
         (*outStream)<<"\"";
         break;
   }

   if (left!=nullptr)
   {
      (*outStream) << ", \"left\":";
      left->writeAsJSON(outStream);
   }

   if (right!=nullptr)
   {
      (*outStream) << ", \"right\":";
      right->writeAsJSON(outStream);
   }

   (*outStream) << "}";
}

TQLExpNode::~TQLExpNode()
{
   if (left!=nullptr)
      delete left;

   if (right!=nullptr)
      delete right;

   switch (opCode)
   {
      case OP_CONST:
         switch (typeDesc->type)
         {
            case TQLType::typeString:
               if (strConstant!=nullptr)
               {
                  delete strConstant;
                  id=nullptr;
               }
               break;
         }
         break;
      case OP_ID:
         if (id!=nullptr)
         {
            delete id;
            id=nullptr;
         }
         break;
   }
   delete typeDesc;
}

double applyOp(int opCode, double lhs, double rhs)
{
   switch (opCode)
   {
      case yy::MyParserBase::token::PLUS:
         return lhs + rhs;
      case yy::MyParserBase::token::MUL:
         return lhs * rhs;
      case yy::MyParserBase::token::DIV:
         return lhs / rhs;
      case yy::MyParserBase::token::MINUS:
         return lhs - rhs;
   }
   return 0;
}

bool applyOp(int opCode, bool lhs, bool rhs)
{
   switch (opCode)
   {
      case yy::MyParserBase::token::EQ:
         return lhs == rhs;
      case yy::MyParserBase::token::NEQ:
         return lhs != rhs;
      case yy::MyParserBase::token::BAND:
         return lhs && rhs;
      case yy::MyParserBase::token::BOR:
         return lhs || rhs;
   }
   return false;
}

bool applyCmpOp(int opCode, double lhs, double rhs)
{
   switch (opCode)
   {
      case yy::MyParserBase::token::LT:
         return lhs < rhs;
      case yy::MyParserBase::token::LTE:
         return lhs <= rhs;
      case yy::MyParserBase::token::GT:
         return lhs > rhs;
      case yy::MyParserBase::token::GTE:
         return lhs >= rhs;
   }
   return false;
}

void extractParameters(vector<TQLVarDesc*> &paramList, TQLExpNode *exp)
{
   if (exp == nullptr) return;

   if (exp->opCode == yy::MyParserBase::token::COMMA)
   {
      paramList.push_back(new TQLVarDesc(exp->right->typeDesc->addRef(), nullptr));
      extractParameters(paramList, exp->left);
   }
   else
   {
      paramList.push_back(new TQLVarDesc(exp->typeDesc->addRef(), nullptr));
   }
}

TQLVarDesc *TQLExpNode::resolveSymbol(TQLIdentifier *id)
{
   if (opCode == yy::MyParserBase::token::DIV)
   {
      if (left != nullptr && right != nullptr && 
          left->typeDesc != nullptr &&
          left->typeDesc->type == TQLType::typeObject ) 
         {
            //TODO
            TQLVarDesc *vd = left->typeDesc->members->findVarStripped(id);
            if (vd != nullptr) return vd;
         }
   }

   if (parent != nullptr)
      return parent->resolveSymbol(id);
   
   return nullptr;
}

void TQLExpNode::computeExpAddr()
{
   switch (opCode)
   {
      case OP_ID:
      {
         TQLVarDesc* vd = resolveSymbol(id);
         if (vd!=nullptr)
            address = vd->address;

         cout << *(id->id) << " " << address << " " << endl;
      }
      break;
      case OP_CALL:
      {
         address = driver->getTQLIR()->funcTable->findIndex(left->id->id);
      }
      break;
   }
   
   if (left != nullptr)
      left->computeExpAddr();

   if (right != nullptr)
      right->computeExpAddr();
}

void TQLExpNode::computeAddresses()
{
   addrOffset = parent != nullptr ? parent->addrOffset + parent->stackSize : 0;
   int addrIndex = addrOffset;
   if (opCode == yy::MyParserBase::token::DIV) 
   {
      if (left->typeDesc->type == TQLType::typeObject && right->typeDesc->type == TQLType::typeBool)
      {
         if (left->typeDesc->members != nullptr)
         {
            for (int j = 0; j < left->typeDesc->members->count(); ++j)
            {
               TQLVarDesc *vdm = left->typeDesc->members->varAt(j);
               vdm->address = addrIndex;
               addrIndex++;
            }
         }
         
      }
   }

   stackSize = addrOffset - addrIndex;

   if (left != nullptr)
      left->computeAddresses();
   
   if (right != nullptr)
      right->computeAddresses();
}

void TQLExpNode::captureVariables()
{
   if (left!=nullptr && opCode != OP_CALL)
   {
      left->captureVariables();
   }

   if (right!=nullptr) 
   {
      right->captureVariables();
   }

   switch (opCode)
   {
      case OP_ID:
         {
            TQLVarDesc *varDesc = resolveSymbol(id);
            if (varDesc != nullptr)
            {
               typeDesc = varDesc->varType != nullptr ? varDesc->varType : new TQLTypeDesc(TQLType::typeNA);
            }
            else 
            {
               typeDesc = new TQLTypeDesc(TQLType::typeNA);
               driver->getTQLIR()->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
                        1, lineno, new string("Symbol could not be resolved.")));
            }
            
            break;
         }
      case OP_CALL:
         {
            vector<TQLVarDesc*> params;
            TQLFuncDesc *func = driver->getTQLIR()->funcTable->findFunc(left->id->id);

            if (func == nullptr)
            {
               typeDesc = new TQLTypeDesc(TQLType::typeNA);
               driver->getTQLIR()->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
                           1, lineno, new string("Function symbol could not be resolved.")));
               break;
            }

            extractParameters(params, right);
            
            if (params.size() != func->parameters->count())
            {
               typeDesc = new TQLTypeDesc(TQLType::typeNA);
               driver->getTQLIR()->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
                           1, lineno, new string("Function parameters does not match.")));
               break;
            }

            for (int i = 0; i < func->parameters->count(); ++i) 
            {
               int i2 =  func->parameters->count() - i - 1;

               if (func->getParameter(i)->varType->type != params[i2]->varType->type)
               {
                  typeDesc = new TQLTypeDesc(TQLType::typeNA);
                  typeDesc = new TQLTypeDesc(TQLType::typeNA);
                  driver->getTQLIR()->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
                           1, lineno, new string("Function parameter types does not match.")));
                  break;
               }
            }

            left->opCode = OP_FID;
            typeDesc = func->retType->addRef();
         }
         break;
      case OP_MUT:
         {

         }
         break;
      case yy::MyParserBase::token::PLUS:
         if (left->typeDesc->type == TQLType::typeNumber &&
             right->typeDesc->type == TQLType::typeNumber)
         {
            if (left->opCode == OP_CONST && right->opCode == OP_CONST)
            {
               opCode = OP_CONST;
               numConstant = left->numConstant + right->numConstant;
               
               delete left; 
               delete right;
               
               left = right = nullptr;
            }
            
            typeDesc = new TQLTypeDesc(TQLType::typeNumber);
         }
         else if (left->typeDesc->type == TQLType::typeString &&
                  right->typeDesc->type == TQLType::typeString)
         {
            if (left->opCode == OP_CONST && right->opCode == OP_CONST)
            {
               opCode = OP_CONST;
               strConstant = new string(*(left->strConstant) + *(right->strConstant));
               
               delete left; 
               delete right;
               
               left = right = nullptr;
            }

            typeDesc = new TQLTypeDesc(TQLType::typeString);
         }
         else 
         {
            typeDesc = new TQLTypeDesc(TQLType::typeNA);
            //TODO: message error
         }
         break;
      case yy::MyParserBase::token::DIV:
         if (left->typeDesc->type == TQLType::typeObject && right->typeDesc->type == TQLType::typeBool)
         {
            typeDesc = new TQLTypeDesc(TQLType::typeObject);
            break;
         }
      case yy::MyParserBase::token::MUL:
         if (left->typeDesc->type == TQLType::typeObject && right->typeDesc->type == TQLType::typeObject)
         {
            typeDesc = new TQLTypeDesc(TQLType::typeObject);
            break;
         }
      case yy::MyParserBase::token::MINUS:
      
         if (left->typeDesc->type == TQLType::typeNumber &&
             right->typeDesc->type == TQLType::typeNumber)
         {
            if (left->opCode == OP_CONST && right->opCode == OP_CONST)
            {
               
               numConstant = applyOp(opCode, left->numConstant, right->numConstant);
               opCode = OP_CONST;

               delete left;
               delete right;
               
               left = right = nullptr;
            }

            typeDesc = new TQLTypeDesc(TQLType::typeNumber);
         }
         else
         {
            typeDesc = new TQLTypeDesc(TQLType::typeNA);
            driver->getTQLIR()->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
                        1, lineno, new string("Type mismatch on arithmetic operator.")));
         }
         break;
      case yy::MyParserBase::token::BAND:
      case yy::MyParserBase::token::BOR:
         if (left->typeDesc->type == TQLType::typeBool &&
             right->typeDesc->type == TQLType::typeBool)
         {
            if (left->opCode == OP_CONST && right->opCode == OP_CONST)
            {
               opCode = OP_CONST;
               boolConstant = applyOp(opCode, left->boolConstant, right->boolConstant);
               
               delete left;
               delete right;
               
               left = right = nullptr;
            }

            typeDesc = new TQLTypeDesc(TQLType::typeBool);
         }
         else
         {
            typeDesc = new TQLTypeDesc(TQLType::typeNA);
            //TODO: message error
         }
         break;
      case yy::MyParserBase::token::EQ:
      case yy::MyParserBase::token::NEQ:
         if (left->typeDesc->type == TQLType::typeBool && right->typeDesc->type == TQLType::typeObject)
         {
            if (left->opCode == OP_CONST && right->opCode == OP_CONST)
            {
               opCode = OP_CONST;
               boolConstant = applyOp(opCode, left->boolConstant, right->boolConstant);
               
               delete left;
               delete right;
               
               left = right = nullptr;
            }

            typeDesc = new TQLTypeDesc(TQLType::typeBool);
         }
      case yy::MyParserBase::token::GTE:
      case yy::MyParserBase::token::GT:
      case yy::MyParserBase::token::LTE:
      case yy::MyParserBase::token::LT:
         if ((left->typeDesc->type == TQLType::typeNumber &&
             right->typeDesc->type == TQLType::typeNumber) ||
             (left->typeDesc->type == TQLType::typeString &&
             right->typeDesc->type == TQLType::typeString))
         {
            if (left->opCode == OP_CONST && right->opCode == OP_CONST)
            {
               opCode = OP_CONST;
               boolConstant = applyCmpOp(opCode, left->boolConstant, right->boolConstant);
               
               delete left;
               delete right;
               
               left = right = nullptr;
            }

            typeDesc = new TQLTypeDesc(TQLType::typeBool);
         }
         else
         {
            typeDesc = new TQLTypeDesc(TQLType::typeNA);
            driver->getTQLIR()->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
                        1, lineno, new string("Type mismatch on comparison operator.")));
         }
         break;
      case yy::MyParserBase::token::ASSIGN:
         if (left->typeDesc->type == right->typeDesc->type) 
         {
            if (left->typeDesc->type == TQLType::typeObject)
            {
               //TODO: handle select.
            }
            typeDesc = left->typeDesc != nullptr ? left->typeDesc : new TQLTypeDesc(TQLType::typeNA);
         }
         else 
         {
            typeDesc = new TQLTypeDesc(TQLType::typeNA);
            driver->getTQLIR()->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
                        1, lineno, new string("Type mismatch on assignment.")));
         }
         break;
   }
}

void TQLExpNode::emitIC(TQLIC *ic)
{
   if (opCode == yy::MyParserBase::token::ASSIGN) 
   {
      if (right != nullptr)
         right->emitIC(ic);
      
      if (left != nullptr) 
      {
         ic->addInstruction(new TQLICInst(OP_SETV, left->address, typeDesc->type));
      }

      return;
   } 
   else if (opCode ==  yy::MyParserBase::token::DIV) 
   {
      if (left->typeDesc->type == TQLType::typeObject && right->typeDesc->type == TQLType::typeBool)
      {
         left->emitIC(ic);
         int p0=ic->count();
         ic->addInstruction(new TQLICInst(OP_CALL, -2, TQLType::typeNA));
         ic->addInstruction(new TQLICInst(OP_POP, 1, TQLType::typeNA));
         
         int p1=ic->count();
         ic->addInstruction(new TQLICInst(OP_JF, -1, TQLType::typeNA));
         right->emitIC(ic);
         ic->addInstruction(new TQLICInst(OP_POP, 1, TQLType::typeNA));
         
         int p2=ic->count();
         ic->addInstruction(new TQLICInst(OP_JF, -1, TQLType::typeNA));

         ic->addInstruction(new TQLICInst(OP_CALL, -8, TQLType::typeNA));
         ic->addInstruction(new TQLICInst(OP_POP, -1, TQLType::typeNA));

         int jf2p1val = ic->count();
         ic->addInstruction(new TQLICInst(OP_CONST, 0, TQLType::typeNumber));

         ic->addInstruction(new TQLICInst(OP_JMP, p0, TQLType::typeNA));
         ic->addInstruction(new TQLICInst(OP_CALL, -9, TQLType::typeNA));
         TQLICInst *inst=ic->instructionAt(p1);
         inst->p1=ic->count();

         inst = ic->instructionAt(p2);
         inst->p1 = jf2p1val;

         return;
      }
   }
   else if (opCode == yy::MyParserBase::token::MUL) 
   {
      if (left->typeDesc->type == TQLType::typeObject && right->typeDesc->type == TQLType::typeObject)
      {
         left->emitIC(ic);
         right->emitIC(ic);
         ic->addInstruction(new TQLICInst(OP_CALL, -5, TQLType::typeNA));
         return;
      }
   }

   if (left!=nullptr)
      left->emitIC(ic);

   if (right!=nullptr)
      right->emitIC(ic);

   if (typeDesc!=nullptr)
   {
      if (typeDesc->type == TQLType::typeObject)
      {
         if (opCode == OP_CONST)
         {
            TQLICInst *inst = new TQLICInst(OP_CALL, -7, TQLType::typeNA);
            ic->addInstruction(inst);
         }
         else if (opCode == OP_MUT)
         {
            TQLICInst *inst = new TQLICInst(OP_CALL, -3, TQLType::typeNA);
            ic->addInstruction(inst);
         }
         else
         {
            TQLICInst *inst = new TQLICInst(opCode, address, typeDesc->type);
            
            ic->addInstruction(inst);
         }
         
      } 
      else 
      {
         TQLICInst *inst = new TQLICInst(opCode, address, typeDesc->type);
         inst->numConstant = numConstant;
         ic->addInstruction(inst);
      }
      
   }
   else
   {
      if (opCode != OP_FID) 
         ic->addInstruction(new TQLICInst(opCode, address, TQLType::typeNA));
   }
      
}


// TQLNode
TQLNode::TQLNode(MyParser *driver) : driver(driver)
{
   lineno = driver->getLine();
}

TQLNode::~TQLNode()
{

}

TQLVarDesc *TQLNode::resolveSymbol(TQLIdentifier* id)
{
   if (parent == nullptr) 
   {
      cout << "node: " << this << " null" << endl;
      return nullptr;
   }
   
   cout << "resolve symbol " << *(id->id) << " node: " << this << endl;
   cout << "parent: " << parent << endl;

   return parent->resolveSymbol(id);
}

// TQLVarList
TQLVarList::TQLVarList()
{

}

bool TQLVarList::addVar(TQLVarDesc *item)
{
   if (findVar(item->id)!=nullptr)
      return false;

   if (vars==nullptr)
      vars=new vector<TQLVarDesc *>();

   vars->push_back(item);

   return true;
}

TQLVarDesc *TQLVarList::findVar(TQLIdentifier *id)
{
   if (vars!=nullptr)
      for (int i=(int)vars->size()-1;i>=0;i--)
         if ((*vars)[i]->id->equals(id))
               return (*vars)[i];

   return nullptr;
}

TQLVarDesc *TQLVarList::findVarStripped(TQLIdentifier *id)
{
   string findId=id->id->substr(1, id->id->length()-1);

   if (vars!=nullptr)
      for (int i=(int)vars->size()-1;i>=0;i--)
         if ((*vars)[i]->id->id->compare(findId)==0)
               return (*vars)[i];

   return nullptr;
}

TQLVarDesc *TQLVarList::varAt(int ndx)
{
   if (vars!=nullptr && ndx>=0 && ndx<(int)vars->size())
      return (*vars)[ndx];

   return nullptr;

}

TQLVarDesc *TQLVarList::operator[] (int ndx)
{
   return (*vars)[ndx];
}

int TQLVarList::count()
{
   if (vars!=nullptr)
      return (int)vars->size();

   return 0;
}

TQLVarList::~TQLVarList()
{
   if (vars!=nullptr)
   {
      for (int i=(int)vars->size()-1;i>=0;i--)
         delete (*vars)[i];

      delete vars;
   }
}

void TQLVarList::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "[";

   if (vars!=nullptr)
   {
      size_t c=vars->size();

      for (int i=0;i<c;i++)
      {
         if (i>0)
            (*outStream) << ", ";

         (*vars)[i]->writeAsJSON(outStream);
      }

   }
   (*outStream) << "]";
}


// TQLVarTable
TQLVarTable::TQLVarTable()
{
}


TQLVarTable::~TQLVarTable()
{
}

// TQLEvaStatement
TQLEvaStatement::TQLEvaStatement(MyParser *driver, TQLExpNode *exp)
   :TQLStatement(driver), exp(exp)
{
   exp->parent = this;
}

TQLEvaStatement::~TQLEvaStatement()
{
   delete exp;
}

void TQLEvaStatement::writeAsJSON(ofstream *outStream)
{
   
   (*outStream) << "{\"statement\": \"evaluation\", \"expression\": ";
   
   exp->writeAsJSON(outStream);
   
   (*outStream) << ", \"lineno\": " << lineno;
   (*outStream) << ", \"this\": \"" << this << "\"";
   (*outStream) << ", \"parent\": \"" << parent << "\"";
   
   (*outStream) << "}";
}

void TQLEvaStatement::computeExpAddr()
{
   exp->computeExpAddr();
}

void TQLEvaStatement::computeAddresses()
{
   addrOffset = parent != nullptr ? parent->addrOffset + parent->stackSize : 0;

   exp->computeAddresses();
}

void TQLEvaStatement::captureVariables()
{
   exp->captureVariables();
}

void TQLEvaStatement::emitIC(TQLIC *ic)
{
   exp->emitIC(ic);
   ic->addInstruction(new TQLICInst(OP_POP, 1, TQLType::typeNA));
}

// TQLLetStatement
TQLLetStatement::TQLLetStatement(MyParser *driver, TQLLetAssignment *letAssignment)
   :TQLStatement(driver)
{
   letAssignments=new vector<TQLLetAssignment *>();
   letAssignments->push_back(letAssignment);
   letAssignment->exp->parent = this;
}

TQLLetStatement::~TQLLetStatement()
{
   if (letAssignments!=nullptr)
   {
      for (int i=(int)letAssignments->size()-1;i>=0;i--)
         delete (*letAssignments)[i];

      delete letAssignments;
   }
}

void TQLLetStatement::appendLetAssignment(TQLLetAssignment *letAssignment)
{
   letAssignments->push_back(letAssignment);
   letAssignment->exp->parent = this;
}

void TQLLetStatement::writeAsJSON(ofstream *outStream)
{
   int cnt=(int)letAssignments->size();

   (*outStream) << "{\"statement\": \"let\", \"assignments\": [";

   for (int i=0;i<cnt;i++)
   {
      TQLLetAssignment *la=(*letAssignments)[i];
      if (i>0)
         (*outStream) << ", ";

      (*outStream) << "{\"id\":\""<< *la->id << "\", \"expression\": ";

      la->exp->writeAsJSON(outStream);

      (*outStream) << "}";
   }
   (*outStream) << "]";

   (*outStream) << ", \"lineno\": " << lineno;
   (*outStream) << ", \"this\": \"" << this << "\"";
   (*outStream) << ", \"parent\": \"" << parent << "\"";

   (*outStream) << "}";
}

void TQLLetStatement::computeExpAddr()
{
   for (TQLLetAssignment *tla : *letAssignments)
   {
      tla->exp->computeExpAddr();
   }
}

void TQLLetStatement::computeAddresses()
{
   addrOffset = parent != nullptr ? parent->addrOffset + parent->stackSize : 0;
   
   for (TQLLetAssignment *tla : *letAssignments)
   {
      tla->exp->computeAddresses();
   }
}

void TQLLetStatement::captureVariables()
{
   if (curTable == nullptr) return;
   
   for (TQLLetAssignment *tla : *letAssignments)
   {
      tla->exp->captureVariables();
      TQLVarDesc *varDesc = new TQLVarDesc(tla->exp->typeDesc, new TQLIdentifier(tla->id));
      if (!curTable->addVar(varDesc))
      {
         driver->getTQLIR()->tqlMsgList->add(new TQLMsg(TQLMsgType::error, 
            1, lineno, new string("Multiple declaration of variable.")));
      }
   }
}

void TQLLetStatement::emitIC(TQLIC *ic)
{
   int cnt=letAssignments->size();

   for (int i=0;i<cnt;i++)
   {
      TQLLetAssignment *la=(*letAssignments)[i];

      TQLVarDesc *vd = resolveSymbol(new TQLIdentifier(la->id));

      la->exp->emitIC(ic);
      ic->addInstruction(new TQLICInst(OP_SETV, vd->address, TQLType::typeNA));
      ic->addInstruction(new TQLICInst(OP_POP, 1, TQLType::typeNA));
   }
}

// TQLAppendStatement
TQLAppendStatement::TQLAppendStatement(MyParser *driver, TQLExpNode *exp)
   : TQLStatement(driver), exp(exp)
{
   exp->parent = this;
}

TQLAppendStatement::~TQLAppendStatement()
{
   delete exp;
}

void TQLAppendStatement::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "{\"statement\": \"append\", \"expression\": ";

   exp->writeAsJSON(outStream);
   (*outStream) << ", \"lineno\": " << lineno;
   (*outStream) << ", \"this\": \"" << this << "\"";
   (*outStream) << ", \"parent\": \"" << parent << "\"";
   (*outStream) << "}";
}

void TQLAppendStatement::computeExpAddr()
{
   exp->computeExpAddr();
}

void TQLAppendStatement::computeAddresses()
{
   addrOffset = parent != nullptr ? parent->addrOffset + parent->stackSize : 0;

   exp->computeAddresses();
}

void TQLAppendStatement::captureVariables()
{
   exp->captureVariables();
}

void TQLAppendStatement::emitIC(TQLIC *ic)
{
   exp->emitIC(ic);
   ic->addInstruction(new TQLICInst(OP_CALL, -1, TQLType::typeNA));
}

// TQLStatement
TQLStatement::TQLStatement(MyParser *driver) : TQLNode(driver)
{
   parent=nullptr;
   vars=nullptr;
}

TQLStatement::~TQLStatement()
{
}

// TQLCompoundStatement
TQLCompoundStatement::TQLCompoundStatement(MyParser *driver)
   :TQLStatement(driver)
{
   stats=nullptr;
   prevTable=nullptr;
   varTable=new TQLVarTable();
}

TQLCompoundStatement::TQLCompoundStatement(MyParser *driver, TQLStatement *statement)
   :TQLStatement(driver)
{
   varTable=new TQLVarTable();
   stats=new vector<TQLStatement *>();

   statement->parent = this;
   stats->push_back(statement);
   prevTable=nullptr;
}

TQLCompoundStatement::~TQLCompoundStatement()
{
   if (stats!=nullptr)
   {
      for (int i=(int)stats->size()-1;i>=0;i--)
         delete (*stats)[i];

      delete stats;
   }
   delete varTable;
}

void TQLCompoundStatement::appendStatement(TQLStatement *statement)
{
   statement->parent = this;
   stats->push_back(statement);
}

void TQLCompoundStatement::writeAsJSON(ofstream *outStream)
{
   if (stats == nullptr) 
   {
       (*outStream) << "{}";
      return;
   }
   int cnt=stats->size();

   (*outStream) << "{\"statement\": \"compound\", \"statements\": [";

   for (int i=0;i<cnt;i++)
   {
      TQLStatement *la=(*stats)[i];
      if (i>0)
         (*outStream) << ", ";

      la->writeAsJSON(outStream);
   }

   (*outStream) << "]";
   (*outStream) << ", \"vartable\": ";

   varTable->writeAsJSON(outStream);
   (*outStream) << ", \"lineno\": " << lineno;
   (*outStream) << ", \"this\": \"" << this << "\"";
   (*outStream) << ", \"parent\": \"" << parent << "\"";
   (*outStream) << "}";
}

TQLVarDesc *TQLCompoundStatement::resolveSymbol(TQLIdentifier *id)
{  
   if (stats == nullptr) {
      if (parent != nullptr)
      {
         return parent->resolveSymbol(id);
      }
      else return nullptr;
   }

   cout << "compound resolve symbol: " << this << endl;
   TQLVarDesc *var = varTable->findVar(id);
   if (var != nullptr)
   {
      return var;
   }
   else if (parent != nullptr)
   {
      return parent->resolveSymbol(id);
   }

   return nullptr;
}

void TQLCompoundStatement::computeExpAddr()
{
   if (stats == nullptr) return;

   for (TQLStatement *st : *stats)
   {
      st->computeExpAddr();
   }
}

void TQLCompoundStatement::computeAddresses()
{
   if (stats == nullptr) return;

   addrOffset = parent != nullptr ? parent->addrOffset + parent->stackSize : 0;
   int addrIndex = addrOffset;
   
   for (int i = 0; i < varTable->count(); ++i)
   {
      TQLVarDesc* vd = varTable->varAt(i);
      vd->address = addrIndex;

      addrIndex++;
   }

   this->stackSize = addrIndex - addrOffset;

   for (TQLStatement *st : *stats)
   {
      st->computeAddresses();
   }
}

void TQLCompoundStatement::captureVariables()
{
   if (stats == nullptr) return;

   int cnt=stats->size();

   prevTable = curTable;
   curTable = this->varTable;
   
   for (int i=0;i<cnt;i++)
   {
      TQLStatement *la=(*stats)[i];
      la->captureVariables();
   }

   curTable = prevTable;
   cout << "Variable counts: " << varTable->count() << endl;
}

void TQLCompoundStatement::emitIC(TQLIC *ic)
{
   if (stats == nullptr) return;
   int cnt=stats->size();

   for (int i=0;i<cnt;i++)
   {
      TQLStatement *la=(*stats)[i];
      la->emitIC(ic);
   }
}

// TQLWhileStatement
TQLWhileStatement::TQLWhileStatement(MyParser *driver, TQLExpNode *exp, TQLStatement *loopStat)
   : TQLStatement(driver), exp(exp), loopStat(loopStat)
{
   exp->parent = this;
   loopStat->parent = this;
}

TQLWhileStatement::~TQLWhileStatement()
{
   delete exp;
   delete loopStat;
}

void TQLWhileStatement::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "{\"statement\": \"while\", \"expression\": ";
   exp->writeAsJSON(outStream);
   (*outStream) << ", \"loopStatement\": ";
   loopStat->writeAsJSON(outStream);
   (*outStream) << ", \"lineno\": " << lineno;
   (*outStream) << ", \"this\": \"" << this << "\"";
   (*outStream) << ", \"parent\": \"" << parent << "\"";
   (*outStream) << "}";
}

void TQLWhileStatement::computeExpAddr()
{
   exp->computeExpAddr();
   loopStat->computeExpAddr();
}


void TQLWhileStatement::computeAddresses()
{
   addrOffset = parent != nullptr ? parent->addrOffset + parent->stackSize : 0;

   exp->computeAddresses();
   loopStat->computeAddresses();
}

void TQLWhileStatement::captureVariables()
{
   exp->captureVariables();
   loopStat->captureVariables();

   //TODO: type of exp must be bool
}

void TQLWhileStatement::emitIC(TQLIC *ic)
{
   int p0=ic->count();
   exp->emitIC(ic);
   ic->addInstruction(new TQLICInst(OP_POP, 1, TQLType::typeNA));

   int p1=ic->count();
   ic->addInstruction(new TQLICInst(OP_JF, 0, TQLType::typeNA));
   loopStat->emitIC(ic);

   ic->addInstruction(new TQLICInst(OP_JMP, p0, TQLType::typeNA));
   TQLICInst *inst=ic->instructionAt(p1);
   inst->p1=ic->count();
}

// TQLIfStat
TQLIfStatement::TQLIfStatement(MyParser *driver, TQLExpNode *exp, TQLStatement *trueStat)
   : TQLStatement(driver), exp(exp), trueStat(trueStat), falseStat(nullptr)
{
   exp->parent = this;
   trueStat->parent = this;
   falseStat->parent = this;
}

TQLIfStatement::TQLIfStatement(MyParser *driver, TQLExpNode *exp, TQLStatement *trueStat, TQLStatement *falseStat)
   : TQLStatement(driver), exp(exp), trueStat(trueStat), falseStat(falseStat)
{
   exp->parent = this;
   trueStat->parent = this;
   
   if (falseStat != nullptr)
      falseStat->parent = this;
}

TQLIfStatement::~TQLIfStatement()
{
   delete exp;
   delete trueStat;

   if (falseStat!=nullptr)
      delete falseStat;
}

void TQLIfStatement::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "{\"statement\": \"if\", \"expression\": ";
   exp->writeAsJSON(outStream);
   (*outStream) << ", \"trueStatement\": ";
   trueStat->writeAsJSON(outStream);

   if (falseStat!=nullptr)
   {
      (*outStream) << ", \"falseStatement\": ";
      falseStat->writeAsJSON(outStream);
   }
   (*outStream) << ", \"lineno\": " << lineno;
   (*outStream) << ", \"this\": \"" << this << "\"";
   (*outStream) << ", \"parent\": \"" << parent << "\"";
   (*outStream) << "}";
}

void TQLIfStatement::computeExpAddr()
{
   exp->computeExpAddr();
   trueStat->computeExpAddr();

   //TODO: type of exp must be bool

   if (falseStat!=nullptr)
   {
      falseStat->computeExpAddr();
   }
}

void TQLIfStatement::computeAddresses()
{
   addrOffset = parent != nullptr ? parent->addrOffset + parent->stackSize : 0;

   exp->computeAddresses();
   trueStat->computeAddresses();

   //TODO: type of exp must be bool

   if (falseStat!=nullptr)
   {
      falseStat->computeAddresses();
   }
}

void TQLIfStatement::captureVariables()
{
   exp->captureVariables();
   trueStat->captureVariables();

   //TODO: type of exp must be bool

   if (falseStat!=nullptr)
   {
      falseStat->captureVariables();
   }
}

void TQLIfStatement::emitIC(TQLIC *ic)
{
   exp->emitIC(ic);
   ic->addInstruction(new TQLICInst(OP_POP, 1, TQLType::typeNA));

   int p1=ic->count();
   ic->addInstruction(new TQLICInst(OP_JF, 0, TQLType::typeNA));
   trueStat->emitIC(ic);

   if (falseStat!=nullptr)
   {
      int p2=ic->count();

      ic->addInstruction(new TQLICInst(OP_JMP, 0, TQLType::typeNA));

      TQLICInst *inst=ic->instructionAt(p1);
      inst->p1=ic->count();
      falseStat->emitIC(ic);

      inst=ic->instructionAt(p2);
      inst->p1=ic->count();
   }
   else
   {
      TQLICInst *inst=ic->instructionAt(p1);
      inst->p1=ic->count();
   }
}

TQLFuncTable::TQLFuncTable()
{
   preset();
}

bool TQLFuncTable::addFunction(TQLFuncDesc *func)
{
   return addVar(func);
}

TQLFuncDesc *TQLFuncTable::findFunc(string *id)
{
   TQLIdentifier ti(new string(*id));

   return (TQLFuncDesc *)findVar(&ti);
}

int TQLFuncTable::findIndex(string *id)
{
   TQLIdentifier ti(new string(*id));

   for (int i = 0; i < count(); ++i) {
      TQLFuncDesc *fd = (TQLFuncDesc*) varAt(i);
      if (fd->id->equals(&ti)) return i;
   }

   return -1;
}

struct FDesc
{
   const char *fName;
   TQLType     typ;
   int         pc;
   struct
   {
       const char   *pName;
       TQLType       typ;
   } para[3];
};
#define FCOUNT 23
void TQLFuncTable::preset()
{
   static FDesc fDesc[23]=
   {
      {"len", TQLType::typeNumber, 1, {{"str", TQLType::typeString}}},
      {"num2str", TQLType::typeString, 1, {{"n", TQLType::typeNumber}}},
      {"str2num", TQLType::typeNumber, 1, {{"str", TQLType::typeString}}},
      {"bool2num", TQLType::typeNumber, 1, {{"b", TQLType::typeBool}}},
      {"num2bool", TQLType::typeBool, 1, {{"n", TQLType::typeNumber}}},
      {"str2bool", TQLType::typeBool, 1, {{"str", TQLType::typeString}}},
      {"bool2str", TQLType::typeString, 1, {{"b", TQLType::typeBool}}},
      {"rowcount", TQLType::typeNumber, 1, {{"t", TQLType::typeObject}}},

      {"sum", TQLType::typeString, 2, {{"t", TQLType::typeObject}, {"n", TQLType::typeNumber}}},
      {"sumofsquares", TQLType::typeString, 2, {{"t", TQLType::typeObject}, {"n", TQLType::typeNumber}}},
      {"max", TQLType::typeString, 2, {{"t", TQLType::typeObject}, {"n", TQLType::typeNumber}}},
      {"min", TQLType::typeString, 2, {{"t", TQLType::typeObject}, {"n", TQLType::typeNumber}}},

      {"sum", TQLType::typeString, 2, {{"t", TQLType::typeObject}, {"fieldId", TQLType::typeString}}},
      {"sumofsquares", TQLType::typeString, 2, {{"t", TQLType::typeObject}, {"fieldId", TQLType::typeString}}},
      {"max", TQLType::typeString, 2, {{"t", TQLType::typeObject}, {"fieldId", TQLType::typeString}}},
      {"min", TQLType::typeString, 2, {{"t", TQLType::typeObject}, {"fieldId", TQLType::typeString}}},

      {"rename", TQLType::typeString, 2, {{"fileName", TQLType::typeString}, {"newFileName", TQLType::typeString}}},
      {"copy", TQLType::typeString, 2, {{"sourceFileName", TQLType::typeString}, {"destFileName", TQLType::typeString}}},
      {"delete", TQLType::typeString, 1, {{"fileName", TQLType::typeString}}},
      {"clearWorkArea", TQLType::typeBool, 0},
      {"int", TQLType::typeNumber, 1, {{"n", TQLType::typeNumber}}},

      {"mid", TQLType::typeString, 3, {{"strFileName", TQLType::typeString}, {"startIndex", TQLType::typeNumber}, {"count", TQLType::typeNumber}}},
      {"random", TQLType::typeNumber, 0}
   };


   TQLFuncDesc *fd;
   FDesc *f=fDesc;;

   for (int i=0;i<FCOUNT;i++, f++)
   {
      fd=new TQLFuncDesc(new TQLIdentifier(new string(f->fName)), new TQLTypeDesc(f->typ));

      addVar(fd);
      for (int j=0;j<f->pc;j++)
         fd->addParameter(new string(f->para[j].pName), new TQLTypeDesc(f->para[j].typ));
   }
}

TQLFuncTable::~TQLFuncTable()
{
}

// TQLFuncDesc
TQLFuncDesc::TQLFuncDesc(TQLIdentifier *id, TQLTypeDesc *retType)
   : TQLVarDesc(retType, id)
{
   parameters=new TQLVarList();
   this->retType = retType;
}

bool TQLFuncDesc::addParameter(string *id, TQLTypeDesc *paraType)
{
   TQLIdentifier *ti=new TQLIdentifier(id);
   TQLVarDesc *vd=new TQLVarDesc(paraType, ti);

   bool retVal=parameters->addVar(vd);

   if (!retVal)
      delete vd;

   return retVal;
}

TQLVarDesc *TQLFuncDesc::getParameter(int ndx)
{
   return parameters->varAt(ndx);
}

TQLFuncDesc::~TQLFuncDesc()
{
   delete parameters;
}

// TQLMsg
TQLMsg::TQLMsg(TQLMsgType msgType, int msgCode, int line, string *msg)
   : msgType(msgType), msgCode(msgCode), line(line), msg(msg)
{
}

TQLMsg::~TQLMsg()
{
   delete msg;
}

void TQLMsg::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "{";

   (*outStream) << "\"code\": " << msgCode;
   (*outStream) << ", \"line\": " << line;
   (*outStream) << ", \"type\": " << ((msgType == TQLMsgType::error) ? "\"Error\"" : ((msgType == TQLMsgType::warning) ? "\"Warning\"" : "\"Info\""));
   (*outStream) << ", \"message\": \"" << *msg << "\"";

   (*outStream) << "}";
}

// TQLMsgList
TQLMsgList::TQLMsgList()
{
   msgList=new vector<TQLMsg *>();
}

TQLMsgList::~TQLMsgList()
{
   int cnt=msgList->size();

   for (int i=0;i<cnt;i++)
      delete (*msgList)[i];

   delete msgList;
}

void TQLMsgList::add(TQLMsg *msg)
{
   msgList->push_back(msg);
}

void TQLMsgList::writeAsJSON(ofstream *outStream)
{
   int cnt=msgList->size();

   (*outStream) << "[";

   for (int i=0;i<cnt;i++)
   {
      (*msgList)[i]->writeAsJSON(outStream);
      if (i != cnt -1) (*outStream) << ", ";
   }
   
   (*outStream) << "]";
}

// TQLICInst
TQLICInst::TQLICInst(int opCode, int p1)
   :opCode(opCode), p1(p1), type(TQLType::typeNA)
{
}

TQLICInst::TQLICInst(int opCode, int p1, TQLType type)
   :opCode(opCode), p1(p1), type(type)
{
}

TQLICInst::~TQLICInst()
{
}

const char *TQLICInst::opStr()
{
   switch (opCode)
   {
      case OP_CONST:
         return "const ";
      case OP_ID:
         return "id    ";
      case OP_CALL:
         return "call()";
      case OP_MUT:
         return "mut[] ";
      case OP_JMP:
         return "jmp   ";
      case OP_JT:
         return "jt    ";
      case OP_JF:
         return "jf    ";
      case OP_SETV:
         return "setv  ";
      case OP_ADDSP:
         return "addsp ";
      case OP_POP:
         return "pop   ";
      case yy::MyParserBase::token::ASSIGN:
         return "asn   ";
      case yy::MyParserBase::token::PLUS:
         return "add   ";
      case yy::MyParserBase::token::MINUS:
         return "sub/neg";
      case yy::MyParserBase::token::DIV:
         return "div   ";
      case yy::MyParserBase::token::MUL:
         return "mul   ";
      case yy::MyParserBase::token::EQ:
         return "equ   ";
      case yy::MyParserBase::token::NEQ:
         return "neq   ";
      case yy::MyParserBase::token::LT:
         return "lt    ";
      case yy::MyParserBase::token::LTE:
         return "lte   ";
      case yy::MyParserBase::token::GT:
         return "gt    ";
      case yy::MyParserBase::token::GTE:
         return "gte   ";
      case yy::MyParserBase::token::BOR:
         return "lor   ";
      case yy::MyParserBase::token::BAND:
         return "land  ";
      case yy::MyParserBase::token::MAT:
         return "mat   ";
   }

   return "unknown";
}

const char *TQLICInst::typeStr() 
{
   switch (type)
   {
      case TQLType::typeNumber:
         return "[number]";
      case TQLType::typeBool:
         return "[bool]";
      case TQLType::typeString:
         return "[string]";
      case TQLType::typeObject:
         return "[object]";
      default:
         return "[unknown type]";
   };

}

string TQLICInst::constStr()
{
     switch (type)
   {
      case TQLType::typeNumber:
         return to_string(numConstant);
      case TQLType::typeBool:
         return boolConstant ? "true" : "false";
      case TQLType::typeString:
         return "\"" + *strConstant + "\"";
      case TQLType::typeObject:
         return "";
      default:
         return "unknown value";
   };

}

void TQLICInst::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "{\"mnemonic\":\"" << opStr() << "\"";
   (*outStream) << ", \"opCode\":" << opCode;
   (*outStream) << ", \"p1\":" << p1;
   (*outStream) << ", \"type\":" << (int)type;
   (*outStream) << "}";
}

void TQLICInst::writeAsTXT(int instno, ofstream *outStream)
{  
   (*outStream) << setw(4) << setfill('0') << instno <<  " " << opStr();
   (*outStream) << "\t" << p1;
   if (type != TQLType::typeNA)
      (*outStream) << "\t" << typeStr() << "\t";
   if (opCode == OP_CONST)
      (*outStream) << constStr();
   (*outStream) << endl;
}

// TQLIC
TQLIC::TQLIC()
{
   code=new vector<TQLICInst *>();
}

TQLIC::~TQLIC()
{
   int cnt=code->size();

   for (int i=0;i<cnt;i++)
      delete (*code)[i];

   delete code;
}

int TQLIC::count()
{
   return (int)code->size();
}

void TQLIC::addInstruction(TQLICInst *inst)
{
   code->push_back(inst);
}

TQLICInst *TQLIC::instructionAt(int loc)
{
   return (*code)[loc];
}

void TQLIC::writeAsJSON(ofstream *outStream)
{
   int cnt=code->size();

   (*outStream) << "[";
   for (int i=0;i<cnt;i++)
   {
      if (i>0)
         (*outStream) << ", ";
      (*code)[i]->writeAsJSON(outStream);
   }
   (*outStream) << "]";
}

void TQLIC::writeAsTXT(ofstream *outStream)
{
   int cnt=code->size();

   for (int i=0;i<cnt;i++)
   {
      (*code)[i]->writeAsTXT(i, outStream);
   }
}


//TQLIR
TQLIR::TQLIR()
{
   tqlMsgList = new TQLMsgList();
   funcTable = new TQLFuncTable(); 
}

TQLIR::~TQLIR()
{

}

void TQLIR::writeAsJSON(ofstream *outStream)
{
   (*outStream) << "{\"messages\":";
   tqlMsgList->writeAsJSON(outStream);

   if (tqlStat!=nullptr)
   {
      (*outStream) << ", \"graphIR\":";
      tqlStat->writeAsJSON(outStream);
   }

   if (tqlIC!=nullptr)
   {
      (*outStream) << ", \"linearIR\":";
      tqlIC->writeAsJSON(outStream);
   }

   (*outStream) << "}";
}

void TQLIR::writeAsTXT(ofstream *outStream)
{
   tqlIC->writeAsTXT(outStream);
}

void TQLIR::computeExpAddr()
{
   tqlStat->computeExpAddr();
}

void TQLIR::computeAddresses()
{
   tqlStat->computeAddresses();
}

void TQLIR::captureVariables()
{
   tqlStat->captureVariables();
}

void TQLIR::emitIC()
{
   // Error count control must be here
   tqlIC=new TQLIC();
   tqlStat->emitIC(tqlIC);
}

TQLTypeDesc *TQLTypeDesc::xProd(TQLTypeDesc *other)
{
   int cnt, i;

   TQLTypeDesc *retVal=new TQLTypeDesc(TQLType::typeObject);
   TQLVarDesc *vd,
              *md;

   cnt=members->count();

   for (i=0;i<cnt;i++)
   {
      vd=members->varAt(i);
      md=new TQLVarDesc(vd->varType, new TQLIdentifier(vd->id->dIndex, new string(*vd->id->id)));

      if (!retVal->addMember(md))
      {
         delete md;
         break;
      }
   }

   if (i==cnt)
   {
      cnt=other->members->count();

      for (i=0;i<cnt;i++)
      {
         vd=other->members->varAt(i);
         md=new TQLVarDesc(vd->varType, new TQLIdentifier(vd->id->dIndex, new string(*vd->id->id)));

         if (!retVal->addMember(md))
         {
            delete md;
            break;
         }
      }
   }

   if (i<cnt)
   {
      delete retVal;
      retVal=nullptr;
   }

   return retVal;
}