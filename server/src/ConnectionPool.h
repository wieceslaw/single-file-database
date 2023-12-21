//
// Created by wieceslaw on 22.12.23.
//

#ifndef SINGLE_FILE_DATABASE_CONNECTIONPOOL_H
#define SINGLE_FILE_DATABASE_CONNECTIONPOOL_H

#include <pthread.h>
#include <stdbool.h>

struct Thread {
    pthread_t pthread;
    bool active;
};

struct ConnectionPool {
    int size;
    void *(*handler)(void *);
    struct Thread* threads;
};

struct ConnectionPool *ConnectionPoolNew(int size, void *(*handler)(void *));

void ConnectionPoolFree(struct ConnectionPool **);

void ConnectionPoolAccept(int sockfd);

#endif //SINGLE_FILE_DATABASE_CONNECTIONPOOL_H
