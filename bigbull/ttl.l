%{
#include <Python.h>
#include "ttldata.h"

int yyFlexLexer::yywrap() { return 1; }

void yyerror(const char *str)
{
    PyErr_SetString(PyExc_SyntaxError, str);
}

%}

%x C_COMMENT C_LONGSTRING C_STRING

SYMBOL [A-Za-z_-][a-zA-Z0-9_-]*
TRIPLEQUOTE \"\"\"

%%

@prefix { LEXER_DATA->add("prefix", "@prefix"); }
# BEGIN(C_COMMENT);
{TRIPLEQUOTE} { LEXER_DATA->strctx.clear(); BEGIN(C_LONGSTRING); }
\" { LEXER_DATA->strctx.clear(); BEGIN(C_STRING); }
\<[^>]*\> { LEXER_DATA->add("URI", std::string(yytext + 1, strlen(yytext) - 2)); }
[+-]?([0-9]+\.[0-9]*|\.[0-9]+)([eE][-+]?[0-9]+)? { std::stringstream ss(yytext); double value; ss >> value; LEXER_DATA->add("number", PyFloat_FromDouble(value)); }
[+-]?[0-9]+ { LEXER_DATA->add("number", PyInt_FromLong(atol(yytext))); }
{SYMBOL}?:{SYMBOL}? { LEXER_DATA->add("prnot", yytext); } 
{SYMBOL} { LEXER_DATA->add("symbol", yytext); } 
[.,;\[\]\(\)] { LEXER_DATA->add(yytext, yytext); } 
[ \t\n] ;

. { 
    std::stringstream ss;
    ss << "Unexpected characters: '" << yytext << "'" << std::endl;
    PyErr_SetString(PyExc_ValueError, ss.str().c_str());
    yyerror("Syntax error");
}

<C_LONGSTRING>{TRIPLEQUOTE} { LEXER_DATA->add("string", LEXER_DATA->strctx); BEGIN(INITIAL); }
<C_LONGSTRING>[^"]+ LEXER_DATA->strctx += yytext;
<C_LONGSTRING>\" LEXER_DATA->strctx += yytext;

<C_STRING>\" { LEXER_DATA->add("string", LEXER_DATA->strctx); BEGIN(INITIAL); }
<C_STRING>\\\" LEXER_DATA->strctx += "\""; 
<C_STRING>[^\\\"\n]+ LEXER_DATA->strctx += yytext;

<C_COMMENT>\n { BEGIN(INITIAL); }
<C_COMMENT>. ;

%%
