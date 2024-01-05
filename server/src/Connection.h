//
// Created by wieceslaw on 22.12.23.
//

#ifndef SINGLE_FILE_DATABASE_CONNECTION_H
#define SINGLE_FILE_DATABASE_CONNECTION_H

#include "Server.h"

struct Connection {
    struct sockaddr_in dest;
    socklen_t socksize;
    pthread_t thread;
    ListIterator node;
    int sockfd;
    struct Server *server;
};

struct Connection *ConnectionNew(struct Server *server);

void ConnectionFree(struct Connection *connection);

int ConnectionStart(struct Connection *connection);

int ConnectionStop(struct Connection *connection);

int ConnectionSendResponse(struct Connection *connection, Response* response);

int ConnectionSendError(struct Connection *connection, char* message);

#endif //SINGLE_FILE_DATABASE_CONNECTION_H
