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

char* CompareTypeToString(enum CompareType type) {
    switch (type) {
        case CMP_LE:
            return "LESS EQUALS";
        case CMP_GE:
            return "GREATER EQUALS";
        case CMP_LS:
            return "LESS";
        case CMP_GR:
            return "GREATER";
        case CMP_EQ:
            return "EQUALS";
        case CMP_NQ:
            return "NOT EQUALS";
    }
}

char* ConditionTypeToString(enum ConditionType type) {
    switch (type) {
        case COND_CMP:
            return "COMPARE";
        case COND_NOT:
            return "NOT";
        case COND_AND:
            return "AND";
        case COND_OR:
            return "OR";
    }
}

void PrintAst(struct AstNode *tree, int indent) {
    if (tree == NULL) {
        return;
    }
    PrintIndent(indent);
    switch (tree->type) {
        case N_INT:
            printf("INT=%d \n", tree->data.INT.value);
            break;
        case N_FLOAT:
            printf("FLOAT=%f \n", tree->data.FLOAT.value);
            break;
        case N_BOOL:
            printf("BOOL=%d \n", tree->data.BOOL.value);
            break;
        case N_STRING:
            printf("STRING=%s \n", tree->data.STRING.value);
            break;
        case N_COMPARE:
            printf("COMPARE: %s \n", CompareTypeToString(tree->data.COMPARE.type));
            PrintAst(tree->data.COMPARE.left, indent + SINGLE_INDENT);
            PrintAst(tree->data.COMPARE.right, indent + SINGLE_INDENT);
            break;
        case N_CONDITION:
            printf("CONDITION: %s \n", ConditionTypeToString(tree->data.CONDITION.type));
            PrintAst(tree->data.CONDITION.first, indent + SINGLE_INDENT);
            PrintAst(tree->data.CONDITION.second, indent + SINGLE_INDENT);
            break;
        case N_OPERAND:
            switch (tree->data.OPERAND.type) {
                case OP_LITERAL:
                    printf("OPERAND: LITERAL: \n");
                    PrintAst(tree->data.OPERAND.LITERAL.value, indent + SINGLE_INDENT);
                    break;
                case OP_COLUMN:
                    printf("OPERAND: TABLE, COLUMN \n");
                    PrintAst(tree->data.OPERAND.COLUMN.table, indent + SINGLE_INDENT);
                    PrintAst(tree->data.OPERAND.COLUMN.column, indent + SINGLE_INDENT);
                    break;
            }
            break;
    }
}

int main() {
    struct AstNode *tree = ParseString("(NOT (1 > 2)) AND (user.id = 1)");
    if (tree == NULL) {
        printf("No tree");
    } else {
        PrintAst(tree, 0);
    }
    FreeAstNode(tree);
    return 0;
}
