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

enum AstNodeType {
    N_INT = 0,
    N_BOOL,
    N_STRING,
    N_FLOAT,
    N_OPERAND,
    N_COMPARE,
    N_CONDITION
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
            char* value;
        } STRING;
        struct {
            float value;
        } FLOAT;
        struct {
            enum OperandType type;
            union {
                struct {
                    struct AstNode* table;
                    struct AstNode* column;
                } COLUMN;
                struct {
                    struct AstNode* value;
                } LITERAL;
            };
        } OPERAND;
        struct {
            enum CompareType type;
            struct AstNode* left;
            struct AstNode* right;
        } COMPARE;
        struct {
            enum ConditionType type;
            struct AstNode* first;
            struct AstNode* second;
        } CONDITION;
    } data;
};

void yyerror(struct AstNode **result, char *s, ...);

struct AstNode *ParseString(char *string);

struct AstNode *ParseFile(FILE *file);

struct AstNode* NewAstNode();

void FreeAstNode(struct AstNode *node);

#endif //LLP_LAB1_AST_H
