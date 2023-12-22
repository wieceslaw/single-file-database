//
// Created by wieceslaw on 22.12.23.
//

#ifndef SINGLE_FILE_DATABASE_TCPSERVER_H
#define SINGLE_FILE_DATABASE_TCPSERVER_H

#include <stdint-gcc.h>
#include <pthread.h>
#include <netinet/in.h>
#include "list/list.h"

struct Server {
    struct sockaddr_in serv;
    socklen_t len;
    uint16_t port;
    int sockfd;
    pthread_t loopThread;
    pthread_mutex_t lock;
    list_t connections;
};

struct Server *ServerNew(uint16_t port);

void ServerFree(struct Server *server);

int ServerStart(struct Server *server);

#endif //SINGLE_FILE_DATABASE_TCPSERVER_H
