%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
extern int yylex();
%}

%parse-param { struct AstNode **tree }

%union {
    struct AstNode *node;
	int ival;
	float fval;
	int bval;
	char* sval;
}

%token<ival> T_INT T_TYPE_BOOL T_TYPE_TEXT T_TYPE_FLOAT32 T_TYPE_INT32
%token<fval> T_FLOAT
%token<sval> T_STRING T_NAME
%token<bval> T_BOOL
%token<ival> T_LESS_EQUALS T_GREATER_EQUALS T_LESS T_GREATER T_EQUALS T_NEQUALS

%token T_LEFT T_RIGHT T_COMMA T_DOT T_SEMICOLON

%token T_NOT T_OR T_AND T_LIMIT T_OFFSET T_VALUES T_INTO T_SET T_CREATE T_TABLE T_ON T_JOIN T_WHERE T_DELETE T_INSERT T_UPDATE T_SELECT T_FROM

%left T_LESS_EQUALS T_GREATER_EQUALS T_LESS T_GREATER T_EQUALS T_NEQUALS

%type<ival> COMPARISON_OPERATOR DATA_TYPE
%type<node> VALUE_INT VALUE_BOOL VALUE_FLOAT VALUE_STRING VALUE
%type<node> COMPARISON_OPERAND COMPARE_CONDITION CONDITION JOIN JOINS WHERE_OPT SELECTOR
%type<node> COLUMN_DECL COLUMNS_DECL
%type<node> COLUMNS_LIST VALUES_LIST UPDATE_LIST TABLE_COLUMN
%type<node> QUERIES QUERY SELECT_QUERY INSERT_QUERY DELETE_QUERY UPDATE_QUERY CREATE_TABLE_QUERY DELETE_TABLE_QUERY

%start QUERIES

%%

QUERIES:
    { $$ = NULL; } |
    QUERY T_SEMICOLON QUERIES {
        $$ = NewListAstNode($1, $3);
        *tree = $$;
    };

QUERY:
    SELECT_QUERY |
    INSERT_QUERY |
    DELETE_QUERY |
    UPDATE_QUERY |
    CREATE_TABLE_QUERY |
    DELETE_TABLE_QUERY;

VALUE_INT: T_INT { $$ = NewIntAstNode($1); };

VALUE_BOOL: T_BOOL { $$ = NewBoolAstNode($1); };

VALUE_FLOAT: T_FLOAT { $$ = NewFloatAstNode($1); };

VALUE_STRING: T_STRING { $$ = NewStringAstNode($1); };

VALUE:
    VALUE_INT |
    VALUE_BOOL |
    VALUE_FLOAT |
    VALUE_STRING;

COMPARISON_OPERATOR:
    T_LESS_EQUALS |
    T_GREATER_EQUALS |
    T_LESS |
    T_GREATER |
    T_EQUALS |
    T_NEQUALS;

TABLE_COLUMN:
    T_NAME T_DOT T_NAME { $$ = NewColumnReferenceAstNode($1, $3); };

COMPARISON_OPERAND:
    TABLE_COLUMN |
    VALUE;

COMPARE_CONDITION:
    COMPARISON_OPERAND COMPARISON_OPERATOR COMPARISON_OPERAND {
        $$ = NewCompareAstNode($2, $1, $3);
    };

CONDITION:
    T_LEFT CONDITION T_RIGHT { $$ = $2; } |
    COMPARE_CONDITION { $$ = NewConditionAstNode(COND_CMP, $1, NULL); } |
    CONDITION T_OR CONDITION { $$ = NewConditionAstNode(COND_OR, $1, $3); } |
    CONDITION T_AND CONDITION { $$ = NewConditionAstNode(COND_AND, $1, $3); } |
    T_NOT CONDITION { $$ = NewConditionAstNode(COND_NOT, $2, NULL); };

JOIN:
    T_JOIN T_NAME T_ON TABLE_COLUMN T_EQUALS TABLE_COLUMN {
        $$ = NewJoinAstNode($2, $4, $6);
    };

JOINS:
    { $$ = NULL; } |
    JOIN JOINS {
        $$ = NewListAstNode($1, $2);
    };

WHERE_OPT: { $$ = NULL; } | T_WHERE CONDITION { $$ = $2; };

SELECTOR:
    TABLE_COLUMN {
        $$ = NewListAstNode($1, NULL);
    } | TABLE_COLUMN T_COMMA SELECTOR {
        $$ = NewListAstNode($1, $3);
    };

SELECT_QUERY:
    T_SELECT SELECTOR T_FROM T_NAME JOINS WHERE_OPT {
        $$ = NewSelectQueryAstNode($2, $4, $5, $6);
    };

DELETE_TABLE_QUERY:
    T_DELETE T_TABLE T_NAME { $$ = NewDeleteTableQueryAstNode($3); };

DATA_TYPE:
    T_TYPE_BOOL |
    T_TYPE_TEXT |
    T_TYPE_FLOAT32 |
    T_TYPE_INT32;

COLUMN_DECL:
    T_NAME DATA_TYPE { $$ = NewColumnDeclarationAstNode($1, $2); };

COLUMNS_DECL:
    COLUMN_DECL {
        $$ = NewListAstNode($1, NULL);
    } |
    COLUMN_DECL T_COMMA COLUMNS_DECL {
        $$ = NewListAstNode($1, $3);
    };

CREATE_TABLE_QUERY:
    T_CREATE T_TABLE T_NAME T_LEFT COLUMNS_DECL T_RIGHT {
        $$ = NewCreateTableQueryAstNode($3, $5);
    };

DELETE_QUERY:
    T_DELETE T_FROM T_NAME WHERE_OPT {
        $$ = NewDeleteQueryAstNode($3, $4);
    };

COLUMNS_LIST:
    T_NAME {
        $$ = NewListAstNode(NewStringAstNode($1), NULL);
    } | T_NAME T_COMMA COLUMNS_LIST {
        $$ = NewListAstNode(NewStringAstNode($1), $3);
    };

VALUES_LIST:
    VALUE {
        $$ = NewListAstNode($1, NULL);
    } | VALUE T_COMMA VALUES_LIST {
        $$ = NewListAstNode($1, $3);
    };

INSERT_QUERY:
    T_INSERT T_INTO T_NAME T_LEFT COLUMNS_LIST T_RIGHT T_VALUES T_LEFT VALUES_LIST T_RIGHT {
        $$ = NewInsertQueryAstNode($3, $5, $9);
    };

UPDATE_LIST:
    T_NAME T_EQUALS VALUE {
        $$ = NewListAstNode(NewUpdateListItemAstNode($1, $3), NULL);
    } | T_NAME T_EQUALS VALUE T_COMMA UPDATE_LIST {
        $$ = NewListAstNode(NewUpdateListItemAstNode($1, $3), $5);
    };

UPDATE_QUERY:
    T_UPDATE T_NAME T_SET UPDATE_LIST WHERE_OPT {
        $$ = NewUpdateQueryAstNode($2, $4, $5);
    };

%%
