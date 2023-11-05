//
// Created by vyach on 04.11.2023.
//

#include "ast.h"

#define SINGLE_INDENT 2

void PrintIndent(int n) {
    for (int i = 0; i < n; i++) {
        printf(" ");
    }
}

void PrintAst(struct AstNode *tree, int indent) {
    PrintIndent(indent);
    switch (tree->type) {
        case N_INT:
            printf("INT=%d \n", tree->data.INT.value);
            break;
        case N_FLOAT:
            printf("N_FLOAT=%f \n", tree->data.FLOAT.value);
            break;
        case N_PLUS:
            printf("PLUS: \n");
            PrintAst(tree->data.PLUS.left, indent + SINGLE_INDENT);
            PrintAst(tree->data.PLUS.right, indent + SINGLE_INDENT);
            break;
        case N_MINUS:
            printf("MINUS: \n");
            PrintAst(tree->data.MINUS.left, indent + SINGLE_INDENT);
            PrintAst(tree->data.MINUS.right, indent + SINGLE_INDENT);
            break;
        case N_MULTIPLY:
            printf("MULTIPLY: \n");
            PrintAst(tree->data.MULTIPLY.left, indent + SINGLE_INDENT);
            PrintAst(tree->data.MULTIPLY.right, indent + SINGLE_INDENT);
            break;
        case N_DIVIDE:
            printf("N_DIVIDE: \n");
            PrintAst(tree->data.DIVIDE.left, indent + SINGLE_INDENT);
            PrintAst(tree->data.DIVIDE.right, indent + SINGLE_INDENT);
            break;
    }
}

int main() {
    struct AstNode *tree = ParseString("1 + (1 * 2) + 213 * 33 - (1 + 2)");
    if (tree == NULL) {
        printf("No tree");
    } else {
        PrintAst(tree, 0);
    }
    return 0;
}
