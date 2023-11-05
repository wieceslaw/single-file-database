%option noyywrap

%{
#include <stdio.h>
#include "parser.h"
%}

%%

[0-9]+\.[0-9]+ 	{yylval.fval = atof(yytext); return T_FLOAT;}
[0-9]+		{yylval.ival = atoi(yytext); return T_INT;}
"+"		{return T_PLUS;}
"-"		{return T_MINUS;}
"*"		{return T_MULTIPLY;}
"/"		{return T_DIVIDE;}
"("		{return T_LEFT;}
")"		{return T_RIGHT;}

[ \n\t]	; // ignore all whitespace
. { printf("Unknown symbol"); };
%%
