//
// Created by wieceslaw on 18.12.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#include "network.h"
#include "DynamicBuffer.h"
#include "ast.h"
#include "defines.h"
#include "util_string.h"
#include "Table.h"

static char *MsgColumnTypeToStr(MsgColumnType type) {
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

static void printTable(MsgTableScheme *scheme) {
    struct Table *table = TableNew(1 + scheme->n_columns, 2);
    TableSet(table, 0, 0, "Column");
    TableSet(table, 0, 1, "Type");
    for (size_t i = 0; i < scheme->n_columns; i++) {
        MsgColumnScheme *column = scheme->columns[i];
        TableSet(table, i + 1, 0, column->name);
        TableSet(table, i + 1, 1, MsgColumnTypeToStr(column->type));
    }
    printf("Table \"%s\" \n", scheme->name);
    TablePrint(table, true);
    TableFree(table);
}

static void printTables(ListTableResponse *response) {
    for (size_t i = 0; i < response->n_tables; i++) {
        MsgTableScheme *table = response->tables[i];
        printTable(table);
        printf("\n");
    }
}

static Message *receive(int sockfd, Response__ContentCase expectedResponseType) {
    Message *message = receiveMessage(sockfd);
    if (message == NULL) {
        debug("Receiving response error \n");
        return NULL;
    }
    if (message->content_case != MESSAGE__CONTENT_RESPONSE) {
        message__free_unpacked(message, NULL);
        debug("Wrong message type");
        return NULL;
    }
    Response *response = message->response;
    if (strcmp(response->error, "") != 0) {
        printf("Server error: %s \n", response->error);
        message__free_unpacked(message, NULL);
        return NULL;
    }
    if (response->content_case != expectedResponseType) {
        message__free_unpacked(message, NULL);
        debug("Wrong response type");
        return NULL;
    }
    return message;
}

static int requestTablesList(int sockfd);

static int requestDeleteTable(int sockfd, struct AstNode *tree);

static int requestCreateTable(int sockfd, struct AstNode *tree);

static int requestInsertQuery(int sockfd, struct AstNode *tree);

static int requestSelectQuery(int sockfd, struct AstNode *tree);

static int requestDeleteQuery(int sockfd, struct AstNode *tree);

static int requestUpdateQuery(int sockfd, struct AstNode *tree);

static size_t AstListLength(struct AstNode *list);

static MsgPredicate *MsgPredicateFromTree(struct AstNode *tree);

static MsgPredicateAnd *MsgPredicateAndFromTree(struct AstNode *tree);

static MsgPredicateNot *MsgPredicateNotFromTree(struct AstNode *tree);

static MsgPredicateOr *MsgPredicateOrFromTree(struct AstNode *tree);

static MsgPredicateCompare *MsgPredicateCompareFromTree(struct AstNode *tree);

static MsgTableScheme *MsgTableSchemeFromTree(struct AstNode *tree);

static Request *CreateTableRequestFromTree(struct AstNode *tree);

static MsgRowData *MsgRowFromTree(struct AstNode *list);

static Request *InsertRequestFromTree(struct AstNode *tree);

static Request *SelectRequestFromTree(struct AstNode *tree);

static Request *UpdateRequestFromTree(struct AstNode *tree);

static MsgColumnData *MsgColumnDataFromTree(struct AstNode *tree);

static int requestQuery(int sockfd, struct AstNode *tree) {
//    PrintAst(tree, 2);
    struct AstNode *node = tree->data.LIST.value;
    int err = 0;
    switch (node->type) {
        case N_DELETE_TABLE_QUERY:
            err = requestDeleteTable(sockfd, node);
            break;
        case N_CREATE_TABLE_QUERY:
            err = requestCreateTable(sockfd, node);
            break;
        case N_INSERT_QUERY:
            err = requestInsertQuery(sockfd, node);
            break;
        case N_SELECT_QUERY:
            err = requestSelectQuery(sockfd, node);
            break;
        case N_DELETE_QUERY:
            err = requestDeleteQuery(sockfd, node);
            break;
        case N_UPDATE_QUERY:
            err = requestUpdateQuery(sockfd, node);
            break;
        default:
            debug("Unexpected query type");
            return -1;
    }
    return err;
}

static void cli(int sockfd) {
    int c;
    int query = 0;
    struct DynamicBuffer buffer = {0};
    printf("database=# ");
    while ((c = getchar()) != 0) {
        if (c == '\n') {
            if (query) {
                printf("database-# ");
            } else {
                printf("database=# ");
            }
        }
        if (query) {
            DynamicBufferPut(&buffer, (char) c);
            if (c == ';') {
                query = 0;
                DynamicBufferPut(&buffer, '\0');
                DynamicBufferReset(&buffer);
                struct AstNode *tree = ParseString(buffer.data);
                if (tree != NULL) {
                    if (requestQuery(sockfd, tree) != 0) {
                        debug("Error while executing request");
                        goto out;
                    }
                    FreeAstNode(tree);
                }
            }
        } else {
            if (c == '\\') {
                c = getchar();
                switch (c) {
                    case 'd': {
                        if (requestTablesList(sockfd) != 0) {
                            debug("Unable to make request");
                            goto out;
                        }
                        break;
                    }
                    case 'q': {
                        goto out;
                    }
                    default: {
                        printf("Try \\? for help. \n");
                    }
                }
            } else if (c != ';' && c != '\n') {
                query = 1;
                DynamicBufferPut(&buffer, (char) c);
            }
        }
    }
    out:
    DynamicBufferFree(&buffer);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments, expected <ip_address> <port> \n");
        return EXIT_FAILURE;
    }
    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    char *addressStr = argv[1];
    char *portStr = argv[2];
    if (parsePort(portStr, &(dest.sin_port))) {
        fprintf(stderr, "Unable to parse port \n");
        return EXIT_FAILURE;
    }
    if (parseIp4Address(addressStr, &dest.sin_addr)) {
        fprintf(stderr, "Unable to parse address \n");
        return EXIT_FAILURE;
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr *) &dest, sizeof(struct sockaddr_in))) {
        fprintf(stderr, "Unable to connect to the server: %s:%s \n", addressStr, portStr);
        return EXIT_FAILURE;
    }

    cli(sockfd);

    close(sockfd);
    return EXIT_SUCCESS;
}

static int requestTablesList(int sockfd) {
    Request request;
    request__init(&request);
    request.content_case = REQUEST__CONTENT_TABLES_LIST;
    ListTableRequest tablesList;
    list_table_request__init(&tablesList);
    tablesList.max = 128;
    request.tableslist = &tablesList;
    sendRequest(sockfd, &request);
    printf("List of relations \n\n");
    Message *msg = receive(sockfd, RESPONSE__CONTENT_TABLE_LIST);
    if (msg == NULL) {
        debug("Response error");
        return -1;
    }
    Response *resp = msg->response;
    printTables(resp->tablelist);
    message__free_unpacked(msg, NULL);
    return 0;
}

static int requestDeleteTable(int sockfd, struct AstNode *tree) {
    char *tableName = tree->data.DELETE_TABLE_QUERY.table;
    Request request;
    request__init(&request);
    request.content_case = REQUEST__CONTENT_DELETE_TABLE;
    DeleteTableRequest deleteTable;
    delete_table_request__init(&deleteTable);
    deleteTable.name = tableName;
    request.deletetable = &deleteTable;
    if (sendRequest(sockfd, &request) != 0) {
        debug("Request error");
        return -1;
    }
    Message *msg = receive(sockfd, RESPONSE__CONTENT_DELETE_TABLE);
    if (msg == NULL) {
        debug("Response error");
        return -1;
    }
    message__free_unpacked(msg, NULL);
    printf("OK \n");
    return 0;
}

static int requestCreateTable(int sockfd, struct AstNode *tree) {
    Request *request = CreateTableRequestFromTree(tree);
    int err = sendRequest(sockfd, request);
    request__free_unpacked(request, NULL);
    if (err != 0) {
        debug("Request error");
        return -1;
    }

    Message *msg = receive(sockfd, RESPONSE__CONTENT_CREATE_TABLE);
    if (msg == NULL) {
        debug("Response error");
        return -1;
    }
    message__free_unpacked(msg, NULL);
    printf("OK \n");
    return 0;
}

static int requestInsertQuery(int sockfd, struct AstNode *tree) {
    Request *request = InsertRequestFromTree(tree);
    int err = sendRequest(sockfd, request);
    request__free_unpacked(request, NULL);
    if (err != 0) {
        debug("error sending request");
        return -1;
    }

    Message *msg = receive(sockfd, RESPONSE__CONTENT_INSERT);
    if (msg == NULL) {
        debug("Response error");
        return -1;
    }
    message__free_unpacked(msg, NULL);

    printf("OK \n");
    return 0;
}

static char* MsgColumnDataToStr(MsgColumnData *column) {
    switch (column->value_case) {
        case MSG_COLUMN_DATA__VALUE_I: {
            char str[128];
            sprintf(str, "%d", column->i);
            return string_copy(str);
        }
        case MSG_COLUMN_DATA__VALUE_F: {
            char str[128];
            sprintf(str, "%f", column->f);
            return string_copy(str);
        }
        case MSG_COLUMN_DATA__VALUE_S:
            return string_copy(column->s);
        case MSG_COLUMN_DATA__VALUE_B: {
            char* str = column->b ? "true" : "false";
            return string_copy(str);
        }
        default:
            debug("Unknown column type");
            assert(0);
    }
}

// select test.int, test.str from test;
static int receiveRows(int sockfd, MsgTableScheme *scheme) {
    size_t maxRows = 100;
    size_t curRow = 1;
    bool headerIsPrinted = false;
    struct Table *table = TableNew(maxRows, scheme->n_columns);
    for (size_t i = 0; i < table->ncols; i++) {
        TableSet(table, 0, i, scheme->columns[i]->name);
    }
    int res = 0;
    while (1) {
        Message *message = receiveMessage(sockfd);
        if (message == NULL) {
            debug("Receiving response error \n");
            res = -1;
            break;
        }
        if (message->content_case != MESSAGE__CONTENT_RESPONSE) {
            message__free_unpacked(message, NULL);
            debug("Wrong message type");
            res = -1;
            break;
        }
        Response *response = message->response;
        if (strcmp(response->error, "") != 0) {
            printf("Server error: %s \n", response->error);
            message__free_unpacked(message, NULL);
            res = -1;
            break;
        }
        if (response->content_case != RESPONSE__CONTENT_BATCH &&
            response->content_case != RESPONSE__CONTENT_BATCH_END) {
            message__free_unpacked(message, NULL);
            debug("Wrong response type");
            res = -1;
            break;
        }
        if (response->content_case == RESPONSE__CONTENT_BATCH_END) {
            for (size_t row = 0; row < curRow; row++) {
                if (!headerIsPrinted && row == 0) {
                    TablePrintBar(table);
                    printf("\n");
                    TablePrintRow(table, row);
                    printf("\n");
                    TablePrintInterBar(table);
                    printf("\n");
                    headerIsPrinted = true;
                } else {
                    TablePrintRow(table, row);
                    printf("\n");
                }
            }
            TablePrintBar(table);
            printf("\n");
            printf("ROWS (%d) \n", response->batchend->count);
            break;
        }
        if (curRow == maxRows) {
            for (size_t row = 0; row < curRow; row++) {
                if (!headerIsPrinted && row == 0) {
                    TablePrintBar(table);
                    printf("\n");
                    TablePrintRow(table, row);
                    printf("\n");
                    TablePrintInterBar(table);
                    printf("\n");
                    headerIsPrinted = true;
                } else {
                    TablePrintRow(table, row);
                    printf("\n");
                }
            }
            curRow = 0;
        }
        for (size_t col = 0; col < table->ncols; col++) {
            MsgRowData *row = response->batch->data;
            char* str = MsgColumnDataToStr(row->columns[col]);
            TableSet(table, curRow, col, str);
            free(str);
        }
        curRow++;
        message__free_unpacked(message, NULL);
    }
    TableFree(table);
    return res;
}

static int requestSelectQuery(int sockfd, struct AstNode *tree) {
    Request *request = SelectRequestFromTree(tree);
    int err = sendRequest(sockfd, request);
    request__free_unpacked(request, NULL);
    if (err != 0) {
        debug("error sending request");
        return -1;
    }

    Message *msg = receive(sockfd, RESPONSE__CONTENT_SELECT);
    if (msg == NULL) {
        debug("Response error");
        return -1;
    }
    receiveRows(sockfd, msg->response->select->scheme);
    message__free_unpacked(msg, NULL);
    return 0;
}

static Request *DeleteRequestFromTree(struct AstNode *tree) {
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

static int requestDeleteQuery(int sockfd, struct AstNode *tree) {
    Request *request = DeleteRequestFromTree(tree);
    int err = sendRequest(sockfd, request);
    request__free_unpacked(request, NULL);
    if (err != 0) {
        debug("Request error");
        return -1;
    }
    Message *msg = receive(sockfd, RESPONSE__CONTENT_DELETE);
    if (msg == NULL) {
        debug("Response error");
        return -1;
    }
    Response *response = msg->response;
    printf("DELETED (%d) \n", response->delete_->count);
    message__free_unpacked(msg, NULL);
    return 0;
}

static SetItem *SetItemFromTree(struct AstNode *tree, char *tableName) {
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

static Request *UpdateRequestFromTree(struct AstNode *tree) {
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

static int requestUpdateQuery(int sockfd, struct AstNode *tree) {
    Request *request = UpdateRequestFromTree(tree);
    int err = sendRequest(sockfd, request);
    request__free_unpacked(request, NULL);
    if (err != 0) {
        debug("Request error");
        return -1;
    }
    Message *msg = receive(sockfd, RESPONSE__CONTENT_UPDATE);
    if (msg == NULL) {
        debug("Response error");
        return -1;
    }
    Response *response = msg->response;
    printf("UPDATED (%d) \n", response->update->count);
    message__free_unpacked(msg, NULL);
    return 0;
}

static Request *CreateTableRequestFromTree(struct AstNode *tree) {
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

static Request *InsertRequestFromTree(struct AstNode *tree) {
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

static size_t AstListLength(struct AstNode *list) {
    size_t length = 0;
    while (list != NULL) {
        length++;
        list = list->data.LIST.next;
    }
    return length;
}

static MsgColumnType MsgColumnTypeFromTree(enum DataType type) {
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

static MsgColumnScheme *MsgColumnSchemeFromTree(struct AstNode *tree) {
    MsgColumnScheme *column = malloc(sizeof(MsgColumnScheme));
    msg_column_scheme__init(column);
    if (column == NULL) {
        return NULL;
    }
    column->name = string_copy(tree->data.COLUMN_DECL.column);
    column->type = MsgColumnTypeFromTree(tree->data.COLUMN_DECL.type);
    return column;
}

static MsgTableScheme *MsgTableSchemeFromTree(struct AstNode *tree) {
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

static MsgColumnData *MsgColumnFromTree(struct AstNode *tree) {
    MsgColumnData *column = malloc(sizeof(MsgColumnData));
    if (column == NULL) {
        return column;
    }
    msg_column_data__init(column);
    switch (tree->type) {
        case N_INT:
            column->i = tree->data.INT.value;
            column->value_case = MSG_COLUMN_DATA__VALUE_I;
            break;
        case N_BOOL:
            column->b = tree->data.BOOL.value;
            column->value_case = MSG_COLUMN_DATA__VALUE_B;
            break;
        case N_STRING:
            column->s = string_copy(tree->data.STRING.value);
            column->value_case = MSG_COLUMN_DATA__VALUE_S;
            break;
        case N_FLOAT:
            column->f = tree->data.FLOAT.value;
            column->value_case = MSG_COLUMN_DATA__VALUE_F;
            break;
        default:
            debug("Unexpected column type");
            assert(0);
    }
    return column;
}

static MsgRowData *MsgRowFromTree(struct AstNode *list) {
    MsgRowData *row = malloc(sizeof(MsgRowData));
    if (row == NULL) {
        return NULL;
    }
    msg_row_data__init(row);
    size_t length = AstListLength(list);
    row->n_columns = length;
    row->columns = malloc(sizeof(MsgColumnData *));
    if (row->columns == NULL) {
        free(row);
        return NULL;
    }
    for (size_t i = 0; i < length; i++) {
        row->columns[i] = MsgColumnFromTree(list->data.LIST.value);
        list = list->data.LIST.next;
    }
    return row;
}

static MsgColumnReference *MsgColumnReferenceFromTree(struct AstNode *node) {
    assert(node->type == N_COLUMN_REFERENCE);
    MsgColumnReference *column = malloc(sizeof(MsgColumnReference));
    msg_column_reference__init(column);
    column->table = string_copy(node->data.COLUMN_REFERENCE.table);
    column->column = string_copy(node->data.COLUMN_REFERENCE.column);
    return column;
}

static Selector *MsgSelectorFromTree(struct AstNode *list) {
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

static MsgColumnData *MsgColumnDataFromTree(struct AstNode *tree) {
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

static MsgPredicateOperand *MsgPredicateOperandFromTree(struct AstNode *tree) {
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

static MsgPredicateAnd *MsgPredicateAndFromTree(struct AstNode *tree) {
    MsgPredicateAnd *predicateAnd = malloc(sizeof(MsgPredicateAnd));
    msg_predicate_and__init(predicateAnd);
    predicateAnd->first = MsgPredicateFromTree(tree->data.CONDITION.first);
    predicateAnd->second = MsgPredicateFromTree(tree->data.CONDITION.second);
    return predicateAnd;
}

static MsgPredicateNot *MsgPredicateNotFromTree(struct AstNode *tree) {
    MsgPredicateNot *predicateNot = malloc(sizeof(MsgPredicateNot));
    msg_predicate_not__init(predicateNot);
    predicateNot->first = MsgPredicateFromTree(tree->data.CONDITION.first);
    return predicateNot;
}

static MsgPredicateOr *MsgPredicateOrFromTree(struct AstNode *tree) {
    MsgPredicateOr *predicateOr = malloc(sizeof(MsgPredicateOr));
    msg_predicate_or__init(predicateOr);
    predicateOr->first = MsgPredicateFromTree(tree->data.CONDITION.first);
    predicateOr->second = MsgPredicateFromTree(tree->data.CONDITION.second);
    return predicateOr;
}

static MsgPredicateCompare *MsgPredicateCompareFromTree(struct AstNode *tree) {
    struct AstNode *compare = tree->data.CONDITION.first;
    assert(compare->type == N_COMPARE);
    MsgPredicateCompare *predicateCmp = malloc(sizeof(MsgPredicateCompare));
    msg_predicate_compare__init(predicateCmp);
    predicateCmp->type = MsgPredicateCompareTypeFromTree(compare->data.COMPARE.type);
    predicateCmp->left = MsgPredicateOperandFromTree(compare->data.COMPARE.left);
    predicateCmp->right = MsgPredicateOperandFromTree(compare->data.COMPARE.right);
    return predicateCmp;
}

static MsgPredicate *MsgPredicateFromTree(struct AstNode *tree) {
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

static JoinCondition *JoinConditionFromTree(struct AstNode *tree) {
    JoinCondition *joinCondition = malloc(sizeof(JoinCondition));
    join_condition__init(joinCondition);
    joinCondition->left = MsgColumnReferenceFromTree(tree->data.JOIN.left);
    joinCondition->right = MsgColumnReferenceFromTree(tree->data.JOIN.right);
    return joinCondition;
}

static Request *SelectRequestFromTree(struct AstNode *tree) {
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
