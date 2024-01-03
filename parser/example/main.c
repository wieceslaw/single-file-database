//
// Created by vyach on 04.11.2023.
//

#include <string.h>
#include <assert.h>
#include "ast.h"

int main(int argc, char * argv[]) {
//    (void)argc;
//    (void)argv;
//    struct AstNode *tree = ParseString("select user.id from user;");
//    if (tree == NULL) {
//        printf("No tree");
//    } else {
//        PrintAst(tree, 0);
//    }
//    return 0;
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
