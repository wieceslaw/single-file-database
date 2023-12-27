//
// Created by wieceslaw on 22.12.23.
//

#ifndef SINGLE_FILE_DATABASE_TCPSERVER_H
#define SINGLE_FILE_DATABASE_TCPSERVER_H

#include <pthread.h>
#include <netinet/in.h>
#include "list/List.h"
#include "DatabaseWrapper.h"

struct Server {
    struct sockaddr_in serv;
    socklen_t len;
    uint16_t port;
    int sockfd;
    pthread_t loopThread;
    pthread_mutex_t lock;
    List connections;
    struct DatabaseWrapper *databaseWrapper;
};

struct Server *ServerNew(uint16_t port, char *filename, file_open_mode mode);

int ServerFree(struct Server *server);

int ServerStart(struct Server *server);

#endif //SINGLE_FILE_DATABASE_TCPSERVER_H
