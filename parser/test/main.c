//
// Created by vyach on 04.11.2023.
//

#include <string.h>
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

char* DataTypeToString(enum DataType type) {
    switch (type) {
        case TYPE_INT32:
            return "INT32";
        case TYPE_FLOAT32:
            return "FLOAT32";
        case TYPE_TEXT:
            return "TEXT";
        case TYPE_BOOL:
            return "BOOLEAN";
    }
}

void PrintAst(struct AstNode *tree, int indent) {
    if (tree == NULL) {
        return;
    }
    PrintIndent(indent);
    switch (tree->type) {
        case N_INT:
            printf("INT [%d] \n", tree->data.INT.value);
            break;
        case N_FLOAT:
            printf("FLOAT [%f] \n", tree->data.FLOAT.value);
            break;
        case N_BOOL:
            printf("BOOL [%s] \n", tree->data.BOOL.value ? "TRUE" : "FALSE");
            break;
        case N_STRING:
            printf("STRING [%s] \n", tree->data.STRING.value);
            break;
        case N_COMPARE:
            printf("COMPARE [TYPE: %s] \n", CompareTypeToString(tree->data.COMPARE.type));
            PrintAst(tree->data.COMPARE.left, indent + SINGLE_INDENT);
            PrintAst(tree->data.COMPARE.right, indent + SINGLE_INDENT);
            break;
        case N_CONDITION:
            printf("CONDITION [TYPE: %s] \n", ConditionTypeToString(tree->data.CONDITION.type));
            PrintAst(tree->data.CONDITION.first, indent + SINGLE_INDENT);
            PrintAst(tree->data.CONDITION.second, indent + SINGLE_INDENT);
            break;
        case N_COLUMN_REFERENCE:
            printf("COLUMN REFERENCE [TABLE: %s, COLUMN: %s] \n", tree->data.COLUMN_REFERENCE.table,
                   tree->data.COLUMN_REFERENCE.column);
            break;
        case N_JOIN:
            printf("JOIN ON [TABLE: %s] \n", tree->data.JOIN.table);
            PrintAst(tree->data.JOIN.left, indent + SINGLE_INDENT);
            PrintAst(tree->data.JOIN.right, indent + SINGLE_INDENT);
            break;
        case N_DELETE_TABLE_QUERY:
            printf("DELETE TABLE [TABLE: %s] \n", tree->data.DELETE_TABLE_QUERY.table);
            break;
        case N_COLUMN_DECL:
            printf("COLUMN DECLARATION [NAME: %s, TYPE: %s] \n", tree->data.COLUMN_DECL.column,
                   DataTypeToString(tree->data.COLUMN_DECL.type));
            break;
        case N_CREATE_TABLE_QUERY:
            printf("CREATE TABLE [TABLE: %s] \n", tree->data.CREATE_TABLE_QUERY.table);
            PrintAst(tree->data.CREATE_TABLE_QUERY.columns, indent + SINGLE_INDENT);
            break;
        case N_SELECT_QUERY:
            printf("SELECT FROM [TABLE: %s] \n", tree->data.SELECT_QUERY.table);
            indent += SINGLE_INDENT;
            PrintIndent(indent);
            printf("SELECTOR LIST \n");
            PrintAst(tree->data.SELECT_QUERY.selector, indent + SINGLE_INDENT);
            PrintIndent(indent);
            printf("JOIN LIST \n");
            PrintAst(tree->data.SELECT_QUERY.join, indent + SINGLE_INDENT);
            PrintIndent(indent);
            printf("WHERE \n");
            PrintAst(tree->data.SELECT_QUERY.where, indent + SINGLE_INDENT);
            break;
        case N_INSERT_QUERY:
            printf("INSERT INTO [TABLE: %s] \n", tree->data.INSERT_QUERY.table);
            indent += SINGLE_INDENT;
            PrintIndent(indent);
            printf("COLUMNS LIST \n");
            PrintAst(tree->data.INSERT_QUERY.columns, indent + SINGLE_INDENT);
            PrintIndent(indent);
            printf("VALUES LIST \n");
            PrintAst(tree->data.INSERT_QUERY.values, indent + SINGLE_INDENT);
            break;
        case N_DELETE_QUERY:
            printf("DELETE FROM [TABLE: %s] \n", tree->data.DELETE_QUERY.table);
            PrintAst(tree->data.DELETE_QUERY.where, indent + SINGLE_INDENT);
            break;
        case N_LIST:
            printf("LIST ITEM \n");
            PrintAst(tree->data.LIST.value, indent + SINGLE_INDENT);
            PrintAst(tree->data.LIST.next, indent);
            break;
        case N_UPDATE_LIST_ITEM:
            printf("SET [COLUMN: %s] \n", tree->data.UPDATE_LIST_ITEM.column);
            PrintAst(tree->data.UPDATE_LIST_ITEM.value, indent + SINGLE_INDENT);
            break;
        case N_UPDATE_QUERY:
            printf("UPDATE [TABLE: %s] \n", tree->data.UPDATE_QUERY.table);
            indent += SINGLE_INDENT;
            PrintIndent(indent);
            printf("UPDATE LIST \n");
            PrintAst(tree->data.UPDATE_QUERY.updateList, indent + SINGLE_INDENT);
            PrintIndent(indent);
            printf("WHERE \n");
            PrintAst(tree->data.UPDATE_QUERY.where, indent + SINGLE_INDENT);
            break;
    }
}

int main(int argc, char * argv[]) {
    if (argc != 2) {
        printf("Wrong number of arguments \n");
        return -1;
    }
    char* filename = argv[1];
    FILE *f = NULL;
    if (strcmp(filename, "stdin") == 0) {
        f = stdin;
    } else {
        f = fopen(filename, "r");
    }
    if (f == NULL) {
        printf("Can't read file: %s \n", filename);
        return -1;
    }
    struct AstNode *tree = ParseFile(f);
    if (tree == NULL) {
        printf("No tree");
    } else {
        PrintAst(tree, 0);
    }
    fclose(f);
    FreeAstNode(tree);
    return 0;
}
