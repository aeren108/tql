using namespace std;

#include <iostream>
#include <fstream>
#include "tqlparse.tab.hh"
#include "FlexLexer.h"
#include "MyFlexLexer.h"

MyFlexLexer::MyFlexLexer(MyParser *pDriver)
{
   driver=pDriver;
}

int yyFlexLexer::yylex()
{
   return -1;
}
