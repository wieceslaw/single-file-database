//
// Created by vyach on 05.11.2023.
//

#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ast.h"

struct AstNode *NewAstNode() {
    struct AstNode *node = malloc(sizeof(struct AstNode));
    if (node == NULL) {
        return NULL;
    }
    *node = (struct AstNode) {0};
    return node;
}

struct AstNode *NewIntAstNode(int value) {
    struct AstNode *node = NewAstNode();
    node->type = N_INT;
    node->data.INT.value = value;
    return node;
}

struct AstNode *NewFloatAstNode(float value) {
    struct AstNode *node = NewAstNode();
    node->type = N_FLOAT;
    node->data.FLOAT.value = value;
    return node;
}

struct AstNode *NewBoolAstNode(bool value) {
    struct AstNode *node = NewAstNode();
    node->type = N_BOOL;
    node->data.BOOL.value = value;
    return node;
}

struct AstNode *NewStringAstNode(char *value) {
    struct AstNode *node = NewAstNode();
    node->type = N_STRING;
    node->data.STRING.value = value;
    return node;
}

struct AstNode *NewColumnReferenceAstNode(char* tableName, char* columnName) {
    struct AstNode *node = NewAstNode();
    node->type = N_COLUMN_REFERENCE;
    node->data.COLUMN_REFERENCE.table = tableName;
    node->data.COLUMN_REFERENCE.column = columnName;
    return node;
}

struct AstNode *NewCompareAstNode(
        enum CompareType type,
        struct AstNode *left,
        struct AstNode *right
) {
    struct AstNode *node = NewAstNode();
    node->type = N_COMPARE;
    node->data.COMPARE.type = type;
    node->data.COMPARE.left = left;
    node->data.COMPARE.right = right;
    return node;
}

struct AstNode *NewConditionAstNode(
        enum ConditionType type,
        struct AstNode *first,
        struct AstNode *second
) {
    struct AstNode *node = NewAstNode();
    node->type = N_CONDITION;
    node->data.CONDITION.type = type;
    node->data.CONDITION.first = first;
    node->data.CONDITION.second = second;
    return node;
}

struct AstNode *NewJoinAstNode(char* table, struct AstNode* left, struct AstNode* right) {
    struct AstNode *node = NewAstNode();
    node->type = N_JOIN;
    node->data.JOIN.table = table;
    node->data.JOIN.left = left;
    node->data.JOIN.right = right;
    return node;
}

struct AstNode *NewDeleteTableQueryAstNode(char *table) {
    struct AstNode *node = NewAstNode();
    node->type = N_DELETE_TABLE_QUERY;
    node->data.DELETE_TABLE_QUERY.table = table;
    return node;
}

struct AstNode *NewCreateTableQueryAstNode(char* table, struct AstNode* columns) {
    struct AstNode *node = NewAstNode();
    node->type = N_CREATE_TABLE_QUERY;
    node->data.CREATE_TABLE_QUERY.table = table;
    node->data.CREATE_TABLE_QUERY.columns = columns;
    return node;
}

struct AstNode *NewSelectQueryAstNode(
        struct AstNode *selector,
        char *table,
        struct AstNode *join,
        struct AstNode *where
) {
    struct AstNode *node = NewAstNode();
    node->type = N_SELECT_QUERY;
    node->data.SELECT_QUERY.table = table;
    node->data.SELECT_QUERY.selector = selector;
    node->data.SELECT_QUERY.join = join;
    node->data.SELECT_QUERY.where = where;
    return node;
}

struct AstNode *NewDeleteQueryAstNode(char *table, struct AstNode *where) {
    struct AstNode *node = NewAstNode();
    node->type = N_DELETE_QUERY;
    node->data.DELETE_QUERY.table = table;
    node->data.DELETE_QUERY.where = where;
    return node;
}

struct AstNode *NewInsertQueryAstNode(
        char *table,
        struct AstNode *columns,
        struct AstNode *values
) {
    struct AstNode *node = NewAstNode();
    node->type = N_INSERT_QUERY;
    node->data.INSERT_QUERY.table = table;
    node->data.INSERT_QUERY.columns = columns;
    node->data.INSERT_QUERY.values = values;
    return node;
}

struct AstNode *NewUpdateQueryAstNode(
        char *table,
        struct AstNode *updateList,
        struct AstNode *where
) {
    struct AstNode *node = NewAstNode();
    node->type = N_UPDATE_QUERY;
    node->data.UPDATE_QUERY.table = table;
    node->data.UPDATE_QUERY.updateList = updateList;
    node->data.UPDATE_QUERY.where = where;
    return node;
}

struct AstNode *NewColumnDeclarationAstNode(char *column, enum DataType type) {
    struct AstNode *node = NewAstNode();
    node->type = N_COLUMN_DECL;
    node->data.COLUMN_DECL.column = column;
    node->data.COLUMN_DECL.type = type;
    return node;
}

struct AstNode* NewListAstNode(struct AstNode *item, struct AstNode *next) {
    struct AstNode *node = NewAstNode();
    node->type = N_LIST;
    node->data.LIST.value = item;
    node->data.LIST.next = next;
    return node;
}

void FreeAstNode(struct AstNode *node) {
    if (node == NULL) {
        return;
    }
    switch (node->type) {
        case N_INT:
            break;
        case N_BOOL:
            break;
        case N_FLOAT:
            break;
        case N_STRING:
            free(node->data.STRING.value);
            break;
        case N_COMPARE:
            FreeAstNode(node->data.COMPARE.right);
            FreeAstNode(node->data.COMPARE.left);
            break;
        case N_CONDITION:
            FreeAstNode(node->data.CONDITION.first);
            FreeAstNode(node->data.CONDITION.second);
            break;
        case N_COLUMN_REFERENCE:
            free(node->data.COLUMN_REFERENCE.table);
            free(node->data.COLUMN_REFERENCE.column);
            break;
        case N_DELETE_TABLE_QUERY:
            free(node->data.DELETE_TABLE_QUERY.table);
            break;
        case N_CREATE_TABLE_QUERY:
            free(node->data.CREATE_TABLE_QUERY.table);
            FreeAstNode(node->data.CREATE_TABLE_QUERY.columns);
            break;
        case N_SELECT_QUERY:
            free(node->data.SELECT_QUERY.table);
            FreeAstNode(node->data.SELECT_QUERY.join);
            FreeAstNode(node->data.SELECT_QUERY.where);
            FreeAstNode(node->data.SELECT_QUERY.selector);
            break;
        case N_INSERT_QUERY:
            FreeAstNode(node->data.INSERT_QUERY.columns);
            FreeAstNode(node->data.INSERT_QUERY.values);
            free(node->data.INSERT_QUERY.table);
            break;
        case N_DELETE_QUERY:
            FreeAstNode(node->data.DELETE_QUERY.where);
            free(node->data.DELETE_QUERY.table);
            break;
        case N_UPDATE_QUERY:
            free(node->data.UPDATE_QUERY.table);
            FreeAstNode(node->data.UPDATE_QUERY.where);
            FreeAstNode(node->data.UPDATE_QUERY.updateList);
            break;
        case N_JOIN:
            FreeAstNode(node->data.JOIN.right);
            FreeAstNode(node->data.JOIN.left);
            free(node->data.JOIN.table);
            break;
        case N_LIST:
            FreeAstNode(node->data.LIST.value);
            FreeAstNode(node->data.LIST.next);
            break;
        case N_UPDATE_LIST_ITEM:
            FreeAstNode(node->data.UPDATE_LIST_ITEM.value);
            free(node->data.UPDATE_LIST_ITEM.column);
            break;
        case N_COLUMN_DECL:
            free(node->data.COLUMN_DECL.column);
            break;
    }
    free(node);
}

extern int yylineno;

typedef struct yy_buffer_state *YY_BUFFER_STATE;

void yy_switch_to_buffer(YY_BUFFER_STATE new_buffer);

extern int yyparse(struct AstNode **result);

extern FILE *yyin;

extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

extern YY_BUFFER_STATE yy_scan_string(char *str);

void yyerror(struct AstNode **result, char *s, ...) {
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    FreeAstNode(*result);
    *result = NULL;
}

struct AstNode *ParseString(char *string) {
    YY_BUFFER_STATE buffer = yy_scan_string(string);
    yy_switch_to_buffer(buffer);
    struct AstNode *nodeRef = NULL;
    yyparse(&nodeRef);
    yy_delete_buffer(buffer);
    return nodeRef;
}

struct AstNode *ParseFile(FILE *file) {
    yyin = file;
    struct AstNode *nodeRef = NULL;
    yyparse(&nodeRef);
    return nodeRef;
}
