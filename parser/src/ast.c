//
// Created by vyach on 05.11.2023.
//

#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ast.h"

struct AstNode* NewAstNode() {
    struct AstNode* node = malloc(sizeof(struct AstNode));
    if (node == NULL) {
        return NULL;
    }
    *node = (struct AstNode) {0};
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
        case N_OPERAND:
            switch (node->data.OPERAND.type) {
                case OP_LITERAL:
                    FreeAstNode(node->data.OPERAND.LITERAL.value);
                    break;
                case OP_COLUMN:
                    FreeAstNode(node->data.OPERAND.COLUMN.table);
                    FreeAstNode(node->data.OPERAND.COLUMN.column);
                    break;
            }
            break;
        case N_CONDITION:
            FreeAstNode(node->data.CONDITION.first);
            FreeAstNode(node->data.CONDITION.second);
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
}

struct AstNode *ParseString(char *string) {
    YY_BUFFER_STATE buffer = yy_scan_string(string);
    yy_switch_to_buffer(buffer);
    struct AstNode* nodeRef = NULL;
    yyparse(&nodeRef);
    yy_delete_buffer(buffer);
    return nodeRef;
}

struct AstNode *ParseFile(FILE *file) {
    yyin = file;
    struct AstNode* nodeRef = NULL;
    yyparse(&nodeRef);
    return nodeRef;
}
