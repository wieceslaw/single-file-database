//
// Created by wieceslaw on 04.01.24.
//

#ifndef SINGLE_FILE_DATABASE_MAPPING_H
#define SINGLE_FILE_DATABASE_MAPPING_H

#include <stddef.h>
#include "predicate.pb-c.h"
#include "request.pb-c.h"
#include "ast.h"

size_t AstListLength(struct AstNode *list);

MsgPredicate *MsgPredicateFromTree(struct AstNode *tree);

MsgPredicateAnd *MsgPredicateAndFromTree(struct AstNode *tree);

MsgPredicateNot *MsgPredicateNotFromTree(struct AstNode *tree);

MsgPredicateOr *MsgPredicateOrFromTree(struct AstNode *tree);

MsgPredicateCompare *MsgPredicateCompareFromTree(struct AstNode *tree);

MsgTableScheme *MsgTableSchemeFromTree(struct AstNode *tree);

MsgRowData *MsgRowFromTree(struct AstNode *list);

Request *CreateTableRequestFromTree(struct AstNode *tree);

Request *InsertQueryRequestFromTree(struct AstNode *tree);

Request *SelectQueryRequestFromTree(struct AstNode *tree);

Request *UpdateQueryRequestFromTree(struct AstNode *tree);

Request *DeleteQueryRequestFromTree(struct AstNode *tree);

MsgColumnData *MsgColumnDataFromTree(struct AstNode *tree);

SetItem *SetItemFromTree(struct AstNode *tree, char *tableName);

char *MsgColumnTypeToStr(MsgColumnType type);

MsgColumnScheme *MsgColumnSchemeFromTree(struct AstNode *tree);

MsgColumnType MsgColumnTypeFromTree(enum DataType type);

MsgPredicateOperand *MsgPredicateOperandFromTree(struct AstNode *tree);

MsgColumnReference *MsgColumnReferenceFromTree(struct AstNode *node);

JoinCondition *JoinConditionFromTree(struct AstNode *tree);

Selector *MsgSelectorFromTree(struct AstNode *list);

#endif //SINGLE_FILE_DATABASE_MAPPING_H
