//
// Created by vyach on 05.11.2023.
//

#ifndef LLP_LAB1_AST_H
#define LLP_LAB1_AST_H

#include <stdio.h>
#include <stdbool.h>

enum AstNodeType {
    N_INT = 0,
    N_FLOAT,
    N_PLUS,
    N_MINUS,
    N_MULTIPLY,
    N_DIVIDE,
};

struct AstNode {
    enum AstNodeType type;
    union {
        struct {
            int value;
        } INT;
        struct {
            float value;
        } FLOAT;
        struct {
            struct AstNode* left;
            struct AstNode* right;
        } PLUS;
        struct {
            struct AstNode* left;
            struct AstNode* right;
        } MINUS;
        struct {
            struct AstNode* left;
            struct AstNode* right;
        } MULTIPLY;
        struct {
            struct AstNode* left;
            struct AstNode* right;
        } DIVIDE;
    } data;
};

extern int yylineno;

void yyerror(struct AstNode **result, char *s, ...);

typedef struct yy_buffer_state *YY_BUFFER_STATE;

extern int yyparse(struct AstNode **result);

extern FILE *yyin;

extern YY_BUFFER_STATE yy_scan_string(char *str);

extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

void yy_switch_to_buffer(YY_BUFFER_STATE new_buffer);

struct AstNode *ParseString(char *string);

struct AstNode *ParseFile(FILE *file);

struct AstNode* NewAstNode();

void FreeAstNode(struct AstNode *node);

#endif //LLP_LAB1_AST_H
