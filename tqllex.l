%option noyywrap
%option yylineno
%{
using namespace std;
#include "tqlparse.tab.hh"

#undef YY_DECL
#define YY_DECL int MyFlexLexer::lex(yy::MyParserBase::semantic_type *lval)

#include "MyFlexLexer.h"
#include "MyParser.h"


%}
digit          [0-9]
pdigit         [1-9]
positiveint    {pdigit}{digit}*
wp             {positiveint}|"0"
fp             ("0"*{pdigit}+)+
number         (({wp}("."{fp})?)|("."{fp}))(("E"|"e")("+"|"-")?{pdigit}{digit}*)?
dpref		   "$"{positiveint}
idstarter      [A-Za-z_]
idchar         [A-Za-z_0-9]
hexdigit       [0-9a-fA-F]
strchar        [\x23-\xff]|[\x20-\x21]
charashex      "\\x"{hexdigit}{hexdigit}?
escaped        "\\n"|"\\r"|"\\"t|"\\\\"|"\\\""
str            \"({strchar}|{charashex}|{escaped})*\"
identifier     "$"?{idstarter}{idchar}*
comment        "//"[^\n]*[\n]
whitesp        [ \n\t\r\n]*
%%
"{"            {return driver->setTerminalSemantics(yy::MyParserBase::token::SOB);}
"}"            {return driver->setTerminalSemantics(yy::MyParserBase::token::EOB);}
"["            {return driver->setTerminalSemantics(yy::MyParserBase::token::OB);}
"]"            {return driver->setTerminalSemantics(yy::MyParserBase::token::CB);}
"\+"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::PLUS);}
"\-"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::MINUS);}
"\*"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::MUL);}
"\/"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::DIV);}
"("            {return driver->setTerminalSemantics(yy::MyParserBase::token::LP);}
")"		       {return driver->setTerminalSemantics(yy::MyParserBase::token::RP);}
","            {return driver->setTerminalSemantics(yy::MyParserBase::token::COMMA);}
"="		       {return driver->setTerminalSemantics(yy::MyParserBase::token::ASSIGN);}
"<"		       {return driver->setTerminalSemantics(yy::MyParserBase::token::LT);}
"<="		   {return driver->setTerminalSemantics(yy::MyParserBase::token::LTE);}
">"		       {return driver->setTerminalSemantics(yy::MyParserBase::token::GT);}
">="		   {return driver->setTerminalSemantics(yy::MyParserBase::token::GTE);}
"=="		   {return driver->setTerminalSemantics(yy::MyParserBase::token::EQ);}
"!="		   {return driver->setTerminalSemantics(yy::MyParserBase::token::NEQ);}
"->"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::MAT);}
"&&"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::BAND);}
"||"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::BOR);}
"!"		       {return driver->setTerminalSemantics(yy::MyParserBase::token::NOT);}
";"			   {return driver->setTerminalSemantics(yy::MyParserBase::token::SEMICOLON);}
"if"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::IF);}
"else"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::ELSE);}
"append"	   {return driver->setTerminalSemantics(yy::MyParserBase::token::APPEND);}
"while"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::WHILE);}
"let"		   {return driver->setTerminalSemantics(yy::MyParserBase::token::LET);}
"@"		       {return driver->setTerminalSemantics(yy::MyParserBase::token::AT);}
"."		       {return driver->setTerminalSemantics(yy::MyParserBase::token::DOT);}
"string"       {return driver->setTerminalSemantics(yy::MyParserBase::token::STRINGTYPE);}
"number"       {return driver->setTerminalSemantics(yy::MyParserBase::token::NUMBERTYPE);}
"boolean"      {return driver->setTerminalSemantics(yy::MyParserBase::token::BOOLTYPE);}
"true"         {return driver->setTerminalSemantics(yy::MyParserBase::token::TRUE);}
"false"        {return driver->setTerminalSemantics(yy::MyParserBase::token::FALSE);}
{number}       {return driver->getNumber();}
{identifier}   {return driver->getId();}
{str}          {return driver->getStr();}
{dpref}  	   {return driver->getDPrefId();}
{comment}
{whitesp}        
.              {return driver->setTerminalSemantics(yy::MyParserBase::token::YYUNDEF);}
%%