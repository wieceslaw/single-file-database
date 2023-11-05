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
        case N_FLOAT:
            break;
        case N_PLUS:
            free(node->data.PLUS.right);
            free(node->data.PLUS.left);
            break;
        case N_MINUS:
            free(node->data.MINUS.right);
            free(node->data.MINUS.left);
            break;
        case N_MULTIPLY:
            free(node->data.MULTIPLY.right);
            free(node->data.MULTIPLY.left);
            break;
        case N_DIVIDE:
            free(node->data.DIVIDE.right);
            free(node->data.DIVIDE.left);
            break;
    }
    free(node);
}

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
    struct AstNode* nodeRef;
    yyparse(&nodeRef);
    yy_delete_buffer(buffer);
    return nodeRef;
}

struct AstNode *ParseFile(FILE *file) {
    yyin = file;
    struct AstNode* nodeRef;
    yyparse(&nodeRef);
    return nodeRef;
}
