%option noyywrap

%{
#include <stdio.h>
#include "parser.h"
#include "ast.h"
%}

%%

[0-9]+\.[0-9]+          { yylval.fval = atof(yytext); return T_FLOAT; }
[0-9]+                  { yylval.ival = atoi(yytext); return T_INT; }
\"(\\.|[^"\\])*\"       { yylval.sval = strdup(yytext); return T_STRING; }
(?i:true)               { yylval.bval = 1; return T_BOOL; }
(?i:false)              { yylval.bval = 0; return T_BOOL; }

(?i:or) return T_OR;
(?i:and) return T_AND;
(?i:not) return T_NOT;

[A-Za-z][A-Za-z0-9_]*   { yylval.sval = strdup(yytext); return T_NAME; }

","     { return T_COMMA; }
"."     { return T_DOT; }
"("		{ return T_LEFT; }
")"		{ return T_RIGHT; }

"<="    { yylval.ival = CMP_LE; return T_LESS_EQUALS; }
">="    { yylval.ival = CMP_GE; return T_GREATER_EQUALS; }
"<"     { yylval.ival = CMP_LS; return T_LESS; }
">"     { yylval.ival = CMP_GR; return T_GREATER; }
"="     { yylval.ival = CMP_EQ; return T_EQUALS; }
"!="    { yylval.ival = CMP_NQ; return T_NEQUALS; }

[ \n\t]	; // ignore all whitespace
. { printf("Unknown symbol"); };
%%
