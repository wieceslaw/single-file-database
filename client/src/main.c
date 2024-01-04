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
#include "mapping.h"

static int requestQuery(int sockfd, struct AstNode *tree);

static int requestTablesList(int sockfd);

static int requestDeleteTable(int sockfd, struct AstNode *tree);

static int requestCreateTable(int sockfd, struct AstNode *tree);

static int requestInsertQuery(int sockfd, struct AstNode *tree);

static int requestSelectQuery(int sockfd, struct AstNode *tree);

static int requestDeleteQuery(int sockfd, struct AstNode *tree);

static int requestUpdateQuery(int sockfd, struct AstNode *tree);

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

static int requestQuery(int sockfd, struct AstNode *tree) {
    struct AstNode *node = tree->data.LIST.value;
    int err;
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
    Request *request = InsertQueryRequestFromTree(tree);
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
    Request *request = SelectQueryRequestFromTree(tree);
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

static int requestDeleteQuery(int sockfd, struct AstNode *tree) {
    Request *request = DeleteQueryRequestFromTree(tree);
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

static int requestUpdateQuery(int sockfd, struct AstNode *tree) {
    Request *request = UpdateQueryRequestFromTree(tree);
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
