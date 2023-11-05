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
%token<sval> T_STRING T_NAME
%token<bval> T_BOOL
%token T_LEFT T_RIGHT T_COMMA T_DOT
%token<ival> T_LESS_EQUALS T_GREATER_EQUALS T_LESS T_GREATER T_EQUALS T_NEQUALS
%token T_NOT T_OR T_AND

%left T_LESS_EQUALS T_GREATER_EQUALS T_LESS T_GREATER T_EQUALS T_NEQUALS

%type<node> TABLE COLUMN VALUE COMPARISON_OPERAND COMPARE_CONDITION CONDITION CONDITIONS
%type<ival> COMPARISON_OPERATOR

%start CONDITIONS

%%

CONDITIONS: | CONDITIONS CONDITION { *tree = $2; };

TABLE:
    T_NAME {
        struct AstNode *node = NewAstNode();
        node->type = N_STRING;
        node->data.STRING.value = $1;
        $$ = node;
    };

COLUMN:
    T_NAME {
        struct AstNode *node = NewAstNode();
        node->type = N_STRING;
        node->data.STRING.value = $1;
        $$ = node;
    };

VALUE:
    T_INT {
        struct AstNode *node = NewAstNode();
        node->type = N_INT;
        node->data.INT.value = $1;
        $$ = node;
    } |
    T_BOOL {
        struct AstNode *node = NewAstNode();
        node->type = N_BOOL;
        node->data.BOOL.value = $1;
        $$ = node;
    } |
    T_FLOAT {
        struct AstNode *node = NewAstNode();
        node->type = N_FLOAT;
        node->data.FLOAT.value = $1;
        $$ = node;
    } |
    T_STRING {
        struct AstNode *node = NewAstNode();
        node->type = N_STRING;
        node->data.STRING.value = $1;
        $$ = node;
    };

COMPARISON_OPERATOR:
    T_LESS_EQUALS |
    T_GREATER_EQUALS |
    T_LESS |
    T_GREATER |
    T_EQUALS |
    T_NEQUALS;

COMPARISON_OPERAND:
    TABLE T_DOT COLUMN {
        struct AstNode *node = NewAstNode();
        node->type = N_OPERAND;
        node->data.OPERAND.type = OP_COLUMN;
        node->data.OPERAND.COLUMN.table = $1;
        node->data.OPERAND.COLUMN.column = $3;
        $$ = node;
    } |
    VALUE {
        struct AstNode *node = NewAstNode();
        node->type = N_OPERAND;
        node->data.OPERAND.type = OP_LITERAL;
        node->data.OPERAND.LITERAL.value = $1;
        $$ = node;
    };

COMPARE_CONDITION:
    COMPARISON_OPERAND COMPARISON_OPERATOR COMPARISON_OPERAND {
        struct AstNode *node = NewAstNode();
        node->type = N_COMPARE;
        node->data.COMPARE.type = $2;
        node->data.COMPARE.left = $1;
        node->data.COMPARE.right = $3;
        $$ = node;
    };

CONDITION:
    T_LEFT CONDITION T_RIGHT {
        $$ = $2;
    } |
    COMPARE_CONDITION {
        struct AstNode *node = NewAstNode();
        node->type = N_CONDITION;
        node->data.CONDITION.type = COND_CMP;
        node->data.CONDITION.first = $1;
        node->data.CONDITION.second = NULL;
        $$ = node;
    }|
    CONDITION T_OR CONDITION {
        struct AstNode *node = NewAstNode();
        node->type = N_CONDITION;
        node->data.CONDITION.type = COND_OR;
        node->data.CONDITION.first = $1;
        node->data.CONDITION.second = $3;
        $$ = node;
    } |
    CONDITION T_AND CONDITION {
        struct AstNode *node = NewAstNode();
        node->type = N_CONDITION;
        node->data.CONDITION.type = COND_AND;
        node->data.CONDITION.first = $1;
        node->data.CONDITION.second = $3;
        $$ = node;
    } |
    T_NOT CONDITION {
        struct AstNode *node = NewAstNode();
        node->type = N_CONDITION;
        node->data.CONDITION.type = COND_NOT;
        node->data.CONDITION.first = $2;
        node->data.CONDITION.second = NULL;
        $$ = node;
    };

%%
