%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "ast.h"
%}

%%

[0-9]+\.[0-9]+          { yylval.fval = atof(yytext); return T_FLOAT; }
[0-9]+                  { yylval.ival = atoi(yytext); return T_INT; }
\"(\\.|[^"\\])*\"       { yylval.sval = strdup(yytext); return T_STRING; }
(?i:true)               { yylval.bval = 1; return T_BOOL; }
(?i:false)              { yylval.bval = 0; return T_BOOL; }

(?i:int32)      { yylval.ival = TYPE_INT32; return T_TYPE_INT32; }
(?i:float32)    { yylval.ival = TYPE_FLOAT32; return T_TYPE_FLOAT32; }
(?i:text)       { yylval.ival = TYPE_TEXT; return T_TYPE_TEXT; }
(?i:boolean)    { yylval.ival = TYPE_BOOL; return T_TYPE_BOOL; }
(?i:from)       { return T_FROM; }
(?i:select)     { return T_SELECT; }
(?i:update)     { return T_UPDATE; }
(?i:insert)     { return T_INSERT; }
(?i:delete)     { return T_DELETE; }
(?i:where)      { return T_WHERE; }
(?i:join)       { return T_JOIN; }
(?i:on)         { return T_ON; }
(?i:table)      { return T_TABLE; }
(?i:create)     { return T_CREATE; }
(?i:set)        { return T_SET; }
(?i:into)       { return T_INTO; }
(?i:values)     { return T_VALUES; }
(?i:offset)     { return T_OFFSET; }
(?i:limit)      { return T_LIMIT; }
(?i:or)         { return T_OR; }
(?i:and)        { return T_AND; }
(?i:not)        { return T_NOT; }

[A-Za-z][A-Za-z0-9_]*   { yylval.sval = strdup(yytext); return T_NAME; }

","     { return T_COMMA; }
"."     { return T_DOT; }
";"     { return T_SEMICOLON; }
"("		{ return T_LEFT; }
")"		{ return T_RIGHT; }

"<="    { yylval.ival = CMP_LE; return T_LESS_EQUALS; }
">="    { yylval.ival = CMP_GE; return T_GREATER_EQUALS; }
"<"     { yylval.ival = CMP_LS; return T_LESS; }
">"     { yylval.ival = CMP_GR; return T_GREATER; }
"="     { yylval.ival = CMP_EQ; return T_EQUALS; }
"!="    { yylval.ival = CMP_NQ; return T_NEQUALS; }

[ \n\t\r]	; // ignore all whitespace
. { printf("Unknown symbol, len: %zu, text[0]: %d \n", strlen(yytext), yytext[0]); };
%%
