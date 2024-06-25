/*
 * MyFlexLexer.h
 *
 *  Created on: Feb 29, 2024
 *      Author: erkan
 */

#ifndef MYFLEXLEXER_H_
#define MYFLEXLEXER_H_

class MyFlexLexer : public yyFlexLexer
{
   MyParser *driver;
public:
   MyFlexLexer(MyParser *pDriver);
   int lex(yy::MyParserBase::semantic_type *lval);
};

#endif /* MYFLEXLEXER_H_ */
