
all: tqlparse.tab.cc lex.yy.cc tqlmain.cpp MyParser.cpp MyFlexLexer.cpp tqlir.cpp
	g++ -g -std=c++11 tqlparse.tab.cc lex.yy.cc tqlmain.cpp MyParser.cpp MyFlexLexer.cpp tqlir.cpp -o tqlp

lex.yy.cc: tqllex.l
	flex -+ tqllex.l

tqlparse.tab.cc: tqlparse.y
	bison --report states --report-file tqlbisonreport.txt -d tqlparse.y

clean:
	rm -rf tqlbisonreport.txt tqlparse.tab.cc tqlparse.tab.hh lex.yy.cc stack.hh tqlp