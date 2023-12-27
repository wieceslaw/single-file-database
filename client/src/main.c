//
// Created by wieceslaw on 18.12.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "network.h"
#include "DynamicBuffer.h"
#include "ast.h"
#include "defines.h"

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

static void printTable(MsgTableScheme *table) {
    printf("Table \"%s\" \n\n", table->name);
    printf("Column \t| Type \n");
    printf("--------+-----\n");
    for (size_t i = 0; i < table->n_columns; i++) {
        MsgColumnScheme *column = table->columns[i];
        printf("%s \t| %s \n", column->name, MsgColumnTypeToStr(column->type));
    }
}

static void printTables(ListTableResponse *response) {
    for (size_t i = 0; i < response->n_tables; i++) {
        MsgTableScheme *table = response->tables[i];
        printTable(table);
    }
}

static int receive(int sockfd) {
    Message *message = receiveMessage(sockfd);
    if (message == NULL) {
        printf("Receiving response error \n");
        return -1;
    }
    if (message->content_case != MESSAGE__CONTENT_RESPONSE) {
        message__free_unpacked(message, NULL);
        debug("Wrong message type");
        return -1;
    }
    Response *response = message->response;
    switch (response->content_case) {
        case RESPONSE__CONTENT_TABLE_LIST:
            printTables(response->tablelist);
            break;
        default:
            debug("Wrong response type");
            assert(0);
    }
    message__free_unpacked(message, NULL);
    return 0;
}

static int requestTablesList(int sockfd) {
    Request request;
    request__init(&request);
    request.content_case = REQUEST__CONTENT_TABLES_LIST;
    ListTableRequest tablesList;
    list_table_request__init(&tablesList);
    tablesList.max = 128;
    request.tableslist = &tablesList;
    return sendRequest(sockfd, &request);
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
                    PrintAst(tree, 2);
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
                        printf("List of relations \n");
                        receive(sockfd);
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
