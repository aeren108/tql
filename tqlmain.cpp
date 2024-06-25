using namespace std;

#include <iostream>
#include <fstream>
#include "tqlparse.tab.hh"
#include "MyParser.h"
#include "FlexLexer.h"
#include "MyFlexLexer.h"

int main(int argc, char **argv)
{
   if (argc==2)
   {
      ifstream is(argv[1]);

      if (is.is_open())
      {
         ofstream os("tqlast.json");
         ofstream tos("tqlir.txt");

         if (os.is_open() && tos.is_open())
         {
            MyParser *driver=new MyParser(&os, &tos);
            yy::MyParserBase *base=new yy::MyParserBase(driver);

            driver->parse(base, &is);

            is.close();

            if (driver->getParseErrorLine()>=0)
            {
               cout << "Not recognized!" << endl;
               driver->reportError();
            }
            else
            {
               cout << "Recognized!" << endl;
               driver->generateIC();
               driver->reportFindings();
            }
            delete driver;
            os.close();
         }
         else
            cout << "Unable to create output file."<<endl;

         is.close();
      }
      else
         cout << "Unable to open input file "<<argv[1]<<endl;
   }
   else
      cout << "Usage is: "<<argv[0]<<" <input-file-name>"<<endl;

   return 0;
}
