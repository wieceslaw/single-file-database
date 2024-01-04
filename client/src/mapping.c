//
// Created by wieceslaw on 04.01.24.
//

#include <malloc.h>
#include "mapping.h"
#include "defines.h"
#include "util_string.h"

char *MsgColumnTypeToStr(MsgColumnType type) {
    switch (type) {
        case MSG_COLUMN_TYPE__INT32:
            return "int";
        case MSG_COLUMN_TYPE__FLOAT32:
            return "float";
        case MSG_COLUMN_TYPE__STRING:
            return "text";
        case MSG_COLUMN_TYPE__BOOL:
            return "boolean";
        default:
            debug("Unknown value of MsgColumnType");
            assert(0);
    }
}

size_t AstListLength(struct AstNode *list) {
    size_t length = 0;
    while (list != NULL) {
        length++;
        list = list->data.LIST.next;
    }
    return length;
}

SetItem *SetItemFromTree(struct AstNode *tree, char *tableName) {
    SetItem *item = malloc(sizeof(SetItem));
    set_item__init(item);
    MsgColumnReference *columnRef = malloc(sizeof(MsgColumnReference));
    msg_column_reference__init(columnRef);
    columnRef->column = string_copy(tree->data.UPDATE_LIST_ITEM.column);
    columnRef->table = string_copy(tableName);
    item->column = columnRef;
    item->value = MsgColumnDataFromTree(tree->data.UPDATE_LIST_ITEM.value);
    return item;
}

Request *UpdateQueryRequestFromTree(struct AstNode *tree) {
    char *table = string_copy(tree->data.UPDATE_QUERY.table);
    MsgPredicate *where = MsgPredicateFromTree(tree->data.UPDATE_QUERY.where);
    struct AstNode *updateList = tree->data.UPDATE_QUERY.updateList;
    size_t length = AstListLength(updateList);

    UpdateRequest *update = malloc(sizeof(UpdateRequest));
    update_request__init(update);
    update->where = where;
    update->table = table;
    update->n_sets = length;
    update->sets = malloc(sizeof(SetItem *) * length);
    for (size_t i = 0; i < length; i++) {
        update->sets[i] = SetItemFromTree(updateList->data.LIST.value, table);
        updateList = updateList->data.LIST.next;
    }

    Request *request = malloc(sizeof(Request));
    request__init(request);
    request->content_case = REQUEST__CONTENT_UPDATE;
    request->update = update;

    return request;
}

Request *CreateTableRequestFromTree(struct AstNode *tree) {
    MsgTableScheme *scheme = MsgTableSchemeFromTree(tree);

    CreateTableRequest *createTable = malloc(sizeof(CreateTableRequest));
    create_table_request__init(createTable);
    createTable->scheme = scheme;

    Request *request = malloc(sizeof(Request));
    request__init(request);
    request->content_case = REQUEST__CONTENT_CREATE_TABLE;
    request->createtable = createTable;

    return request;
}

MsgColumnType MsgColumnTypeFromTree(enum DataType type) {
    switch (type) {
        case TYPE_INT32:
            return MSG_COLUMN_TYPE__INT32;
        case TYPE_FLOAT32:
            return MSG_COLUMN_TYPE__FLOAT32;
        case TYPE_TEXT:
            return MSG_COLUMN_TYPE__STRING;
        case TYPE_BOOL:
            return MSG_COLUMN_TYPE__BOOL;
        default:
            debug("Unknown tree data type");
            assert(0);
    }
}

MsgColumnScheme *MsgColumnSchemeFromTree(struct AstNode *tree) {
    MsgColumnScheme *column = malloc(sizeof(MsgColumnScheme));
    msg_column_scheme__init(column);
    if (column == NULL) {
        return NULL;
    }
    column->name = string_copy(tree->data.COLUMN_DECL.column);
    column->type = MsgColumnTypeFromTree(tree->data.COLUMN_DECL.type);
    return column;
}

MsgTableScheme *MsgTableSchemeFromTree(struct AstNode *tree) {
    MsgTableScheme *scheme = malloc(sizeof(MsgTableScheme));
    msg_table_scheme__init(scheme);
    if (scheme == NULL) {
        return NULL;
    }
    scheme->name = string_copy(tree->data.CREATE_TABLE_QUERY.table);
    if (scheme->name == NULL) {
        free(scheme);
        return NULL;
    }
    struct AstNode *list = tree->data.CREATE_TABLE_QUERY.columns;
    size_t length = AstListLength(list);
    scheme->n_columns = length;
    scheme->columns = malloc(sizeof(MsgColumnScheme *) * length);
    if (scheme->columns == NULL) {
        free(scheme->name);
        free(scheme);
        return NULL;
    }
    for (size_t i = 0; i < length; i++) {
        scheme->columns[i] = MsgColumnSchemeFromTree(list->data.LIST.value);
        list = list->data.LIST.next;
    }
    return scheme;
}

MsgRowData *MsgRowFromTree(struct AstNode *list) {
    MsgRowData *row = malloc(sizeof(MsgRowData));
    if (row == NULL) {
        return NULL;
    }
    msg_row_data__init(row);
    size_t length = AstListLength(list);
    row->n_columns = length;
    row->columns = malloc(sizeof(MsgColumnData *) * length);
    if (row->columns == NULL) {
        free(row);
        return NULL;
    }
    for (size_t i = 0; i < length; i++) {
        row->columns[i] = MsgColumnDataFromTree(list->data.LIST.value);
        list = list->data.LIST.next;
    }
    return row;
}

MsgColumnData *MsgColumnDataFromTree(struct AstNode *tree) {
    MsgColumnData *data = malloc(sizeof(MsgColumnData));
    msg_column_data__init(data);
    switch (tree->type) {
        case N_INT:
            data->value_case = MSG_COLUMN_DATA__VALUE_I;
            data->i = tree->data.INT.value;
            break;
        case N_BOOL:
            data->value_case = MSG_COLUMN_DATA__VALUE_B;
            data->b = tree->data.BOOL.value;
            break;
        case N_STRING:
            data->value_case = MSG_COLUMN_DATA__VALUE_S;
            data->s = string_copy(tree->data.STRING.value);
            break;
        case N_FLOAT:
            data->value_case = MSG_COLUMN_DATA__VALUE_F;
            data->f = tree->data.FLOAT.value;
            break;
        default:
            debug("Unexpected node type");
            assert(0);
    }
    return data;
}

static MsgPredicateCompareType MsgPredicateCompareTypeFromTree(enum CompareType type) {
    switch (type) {
        case CMP_LE:
            return MSG_PREDICATE_COMPARE_TYPE__CMP_LE;
        case CMP_GE:
            return MSG_PREDICATE_COMPARE_TYPE__CMP_GE;
        case CMP_LS:
            return MSG_PREDICATE_COMPARE_TYPE__CMP_LS;
        case CMP_GR:
            return MSG_PREDICATE_COMPARE_TYPE__CMP_GR;
        case CMP_EQ:
            return MSG_PREDICATE_COMPARE_TYPE__CMP_EQ;
        case CMP_NQ:
            return MSG_PREDICATE_COMPARE_TYPE__CMP_NQ;
        default:
            debug("Unknown compare type");
            assert(0);
    }
}

MsgPredicateAnd *MsgPredicateAndFromTree(struct AstNode *tree) {
    MsgPredicateAnd *predicateAnd = malloc(sizeof(MsgPredicateAnd));
    msg_predicate_and__init(predicateAnd);
    predicateAnd->first = MsgPredicateFromTree(tree->data.CONDITION.first);
    predicateAnd->second = MsgPredicateFromTree(tree->data.CONDITION.second);
    return predicateAnd;
}

MsgPredicateNot *MsgPredicateNotFromTree(struct AstNode *tree) {
    MsgPredicateNot *predicateNot = malloc(sizeof(MsgPredicateNot));
    msg_predicate_not__init(predicateNot);
    predicateNot->first = MsgPredicateFromTree(tree->data.CONDITION.first);
    return predicateNot;
}

MsgPredicateOr *MsgPredicateOrFromTree(struct AstNode *tree) {
    MsgPredicateOr *predicateOr = malloc(sizeof(MsgPredicateOr));
    msg_predicate_or__init(predicateOr);
    predicateOr->first = MsgPredicateFromTree(tree->data.CONDITION.first);
    predicateOr->second = MsgPredicateFromTree(tree->data.CONDITION.second);
    return predicateOr;
}

MsgPredicateCompare *MsgPredicateCompareFromTree(struct AstNode *tree) {
    struct AstNode *compare = tree->data.CONDITION.first;
    assert(compare->type == N_COMPARE);
    MsgPredicateCompare *predicateCmp = malloc(sizeof(MsgPredicateCompare));
    msg_predicate_compare__init(predicateCmp);
    predicateCmp->type = MsgPredicateCompareTypeFromTree(compare->data.COMPARE.type);
    predicateCmp->left = MsgPredicateOperandFromTree(compare->data.COMPARE.left);
    predicateCmp->right = MsgPredicateOperandFromTree(compare->data.COMPARE.right);
    return predicateCmp;
}

MsgPredicateOperand *MsgPredicateOperandFromTree(struct AstNode *tree) {
    MsgPredicateOperand *operand = malloc(sizeof(MsgPredicateOperand));
    msg_predicate_operand__init(operand);
    if (tree->type == N_COLUMN_REFERENCE) {
        operand->content_case = MSG_PREDICATE_OPERAND__CONTENT_COLUMN;
        operand->column = MsgColumnReferenceFromTree(tree);
    } else {
        operand->content_case = MSG_PREDICATE_OPERAND__CONTENT_LITERAL;
        operand->literal = MsgColumnDataFromTree(tree);
    }
    return operand;
}

MsgPredicate *MsgPredicateFromTree(struct AstNode *tree) {
    if (tree == NULL) {
        return NULL;
    }
    MsgPredicate *predicate = malloc(sizeof(MsgPredicate));
    msg_predicate__init(predicate);
    switch (tree->data.CONDITION.type) {
        case COND_CMP:
            predicate->content_case = MSG_PREDICATE__CONTENT_COMPARE;
            predicate->compare = MsgPredicateCompareFromTree(tree);
            break;
        case COND_NOT:
            predicate->content_case = MSG_PREDICATE__CONTENT_NOT;
            predicate->not_ = MsgPredicateNotFromTree(tree);
            break;
        case COND_AND:
            predicate->content_case = MSG_PREDICATE__CONTENT_AND;
            predicate->and_ = MsgPredicateAndFromTree(tree);
            break;
        case COND_OR:
            predicate->content_case = MSG_PREDICATE__CONTENT_OR;
            predicate->or_ = MsgPredicateOrFromTree(tree);
            break;
        default:
            debug("Unknown condition type");
            assert(0);
    }
    return predicate;
}

MsgColumnReference *MsgColumnReferenceFromTree(struct AstNode *node) {
    assert(node->type == N_COLUMN_REFERENCE);
    MsgColumnReference *column = malloc(sizeof(MsgColumnReference));
    msg_column_reference__init(column);
    column->table = string_copy(node->data.COLUMN_REFERENCE.table);
    column->column = string_copy(node->data.COLUMN_REFERENCE.column);
    return column;
}

Selector *MsgSelectorFromTree(struct AstNode *list) {
    if (list == NULL) {
        return NULL;
    }
    Selector *selector = malloc(sizeof(Selector));
    selector__init(selector);
    size_t length = AstListLength(list);
    selector->n_columns = length;
    selector->columns = malloc(sizeof(MsgColumnReference *) * length);
    for (size_t i = 0; i < length; i++) {
        selector->columns[i] = MsgColumnReferenceFromTree(list->data.LIST.value);
        list = list->data.LIST.next;
    }
    return selector;
}

JoinCondition *JoinConditionFromTree(struct AstNode *tree) {
    JoinCondition *joinCondition = malloc(sizeof(JoinCondition));
    join_condition__init(joinCondition);
    joinCondition->left = MsgColumnReferenceFromTree(tree->data.JOIN.left);
    joinCondition->right = MsgColumnReferenceFromTree(tree->data.JOIN.right);
    return joinCondition;
}

Request *SelectQueryRequestFromTree(struct AstNode *tree) {
    SelectRequest *select = malloc(sizeof(SelectRequest));
    select_request__init(select);

    char *table = string_copy(tree->data.SELECT_QUERY.table);
    Selector *selector = MsgSelectorFromTree(tree->data.SELECT_QUERY.selector);
    MsgPredicate *where = MsgPredicateFromTree(tree->data.SELECT_QUERY.where);

    select->where = where;
    select->selector = selector;
    select->table = table;

    struct AstNode *joinList = tree->data.SELECT_QUERY.join;
    size_t joinListLength = AstListLength(joinList);
    select->n_joins = joinListLength;
    select->joins = malloc(sizeof(JoinCondition *) * joinListLength);
    for (size_t i = 0; i < joinListLength; i++) {
        select->joins[i] = JoinConditionFromTree(joinList->data.LIST.value);
        joinList = joinList->data.LIST.next;
    }

    Request *request = malloc(sizeof(Request));
    request__init(request);
    request->content_case = REQUEST__CONTENT_SELECT;
    request->select = select;

    return request;
}

Request *InsertQueryRequestFromTree(struct AstNode *tree) {
    char *tableName = string_copy(tree->data.INSERT_QUERY.table);
    struct AstNode *list = tree->data.INSERT_QUERY.values;
    MsgRowData *row = MsgRowFromTree(list);
    MsgRowData **rows = malloc(sizeof(MsgRowData *) * 1);
    rows[0] = row;

    InsertRequest *insert = malloc(sizeof(InsertRequest));
    insert_request__init(insert);
    insert->n_values = 1;
    insert->table = tableName;
    insert->values = rows;

    Request *request = malloc(sizeof(Request));
    request__init(request);
    request->content_case = REQUEST__CONTENT_INSERT;
    request->insert = insert;

    return request;
}

Request *DeleteQueryRequestFromTree(struct AstNode *tree) {
    char *table = string_copy(tree->data.DELETE_QUERY.table);
    MsgPredicate *where = MsgPredicateFromTree(tree->data.DELETE_QUERY.where);

    DeleteRequest *delete = malloc(sizeof(DeleteRequest));
    delete_request__init(delete);
    delete->where = where;
    delete->table = table;

    Request *request = malloc(sizeof(Request));
    request__init(request);
    request->content_case = REQUEST__CONTENT_DELETE;
    request->delete_ = delete;

    return request;
}
