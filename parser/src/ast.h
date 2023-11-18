//
// Created by vyach on 05.11.2023.
//

#ifndef LLP_LAB1_AST_H
#define LLP_LAB1_AST_H

#include <stdio.h>
#include <stdbool.h>

enum CompareType {
    CMP_LE = 0,
    CMP_GE,
    CMP_LS,
    CMP_GR,
    CMP_EQ,
    CMP_NQ,
};

enum ConditionType {
    COND_CMP = 0,
    COND_NOT,
    COND_AND,
    COND_OR,
};

enum OperandType {
    OP_LITERAL = 0,
    OP_COLUMN,
};

enum DataType {
    TYPE_INT32 = 0,
    TYPE_FLOAT32,
    TYPE_TEXT,
    TYPE_BOOL,
};

enum AstNodeType {
    N_INT = 0,
    N_BOOL,
    N_STRING,
    N_FLOAT,
    N_COLUMN_REFERENCE,
    N_COMPARE,
    N_CONDITION,
    N_JOIN,
    N_DELETE_TABLE_QUERY,
    N_COLUMN_DECL,
    N_CREATE_TABLE_QUERY,
    N_SELECT_QUERY,
    N_INSERT_QUERY,
    N_DELETE_QUERY,
    N_LIST,
    N_UPDATE_LIST_ITEM,
    N_UPDATE_QUERY,
};

struct AstNode {
    enum AstNodeType type;
    union {
        struct {
            int value;
        } INT;
        struct {
            bool value;
        } BOOL;
        struct {
            char *value;
        } STRING;
        struct {
            float value;
        } FLOAT;
        struct {
            char *table;
            char *column;
        } COLUMN_REFERENCE;
        struct {
            enum CompareType type;
            struct AstNode *left;
            struct AstNode *right;
        } COMPARE;
        struct {
            enum ConditionType type;
            struct AstNode *first;
            struct AstNode *second;
        } CONDITION;
        struct {
            char *table;
            struct AstNode *left;
            struct AstNode *right;
        } JOIN;
        struct {
            char *table;
        } DELETE_TABLE_QUERY;
        struct {
            enum DataType type;
            char *column;
        } COLUMN_DECL;
        struct {
            char *table;
            struct AstNode *columns;
        } CREATE_TABLE_QUERY;
        struct {
            struct AstNode *selector;
            char *table;
            struct AstNode *join;
            struct AstNode *where;
        } SELECT_QUERY;
        struct {
            char *table;
            struct AstNode *columns;
            struct AstNode *values;
        } INSERT_QUERY;
        struct {
            char *table;
            struct AstNode *where;
        } DELETE_QUERY;
        struct {
            // TODO: change or not? Complex structures inside ast may be a bad idea.
            struct AstNode *value;
            struct AstNode *next;
        } LIST;
        struct {
            char* column;
            struct AstNode *value;
        } UPDATE_LIST_ITEM;
        struct {
            char *table;
            struct AstNode *updateList;
            struct AstNode *where;
        } UPDATE_QUERY;
    } data;
};

void yyerror(struct AstNode **result, char *s, ...);

struct AstNode *ParseString(char *string);

struct AstNode *ParseFile(FILE *file);

void FreeAstNode(struct AstNode *node);

struct AstNode *NewAstNode();

struct AstNode *NewIntAstNode(int value);

struct AstNode *NewFloatAstNode(float value);

struct AstNode *NewBoolAstNode(bool value);

struct AstNode *NewStringAstNode(char *value);

struct AstNode *NewColumnReferenceAstNode(char *tableName, char *columnName);

struct AstNode *NewCompareAstNode(
        enum CompareType type,
        struct AstNode *left,
        struct AstNode *right
);

struct AstNode *NewConditionAstNode(
        enum ConditionType type,
        struct AstNode *first,
        struct AstNode *second
);

struct AstNode *NewJoinAstNode(char *table, struct AstNode *left, struct AstNode *right);

struct AstNode *NewDeleteTableQueryAstNode(char *table);

struct AstNode *NewColumnDeclarationAstNode(char *column, enum DataType type);

struct AstNode *NewCreateTableQueryAstNode(char *table, struct AstNode *columns);

struct AstNode *NewSelectQueryAstNode(
        struct AstNode *selector,
        char *table,
        struct AstNode *join,
        struct AstNode *where
);

struct AstNode *NewDeleteQueryAstNode(char *table, struct AstNode *where);

struct AstNode *NewInsertQueryAstNode(
        char *table,
        struct AstNode *columns,
        struct AstNode *values
);

struct AstNode *NewUpdateQueryAstNode(
        char *table,
        struct AstNode *updateList,
        struct AstNode *where
);

struct AstNode* NewListAstNode(struct AstNode *item, struct AstNode *next);

#endif //LLP_LAB1_AST_H
