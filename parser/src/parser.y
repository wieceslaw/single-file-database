%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
extern int yylex();
%}

%parse-param {struct AstNode **tree}

%union {
    struct AstNode *node;
	int ival;
	float fval;
	int bval;
	char* sval;
}

%token<ival> T_INT
%token<fval> T_FLOAT
%token T_LEFT T_RIGHT T_PLUS T_MINUS T_MULTIPLY T_DIVIDE
%left T_PLUS T_MINUS T_MULTIPLY T_DIVIDE
%type<node> statement
%start statements

%%

statements: | statements statement { *tree = $2; };

statement: T_INT {
    struct AstNode *node = NewAstNode();
    node->type = N_INT;
    node->data.INT.value = $1;
    $$ = node;
} |
statement T_PLUS statement {
    struct AstNode *node = NewAstNode();
    node->type = N_PLUS;
    node->data.PLUS.left = $1;
    node->data.PLUS.right = $3;
    $$ = node;
} |
statement T_MINUS statement	{
    struct AstNode *node = NewAstNode();
    node->type = N_MINUS;
    node->data.MINUS.left = $1;
    node->data.MINUS.right = $3;
    $$ = node;
} |
statement T_MULTIPLY statement {
    struct AstNode *node = NewAstNode();
    node->type = N_MULTIPLY;
    node->data.MULTIPLY.left = $1;
    node->data.MULTIPLY.right = $3;
    $$ = node;
} |
T_LEFT statement T_RIGHT {
    $$ = $2;
};

%%
