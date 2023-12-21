//
// Created by wieceslaw on 22.12.23.
//

#include "ConnectionPool.h"

struct ConnectionPool *ConnectionPoolNew(int size, void *(*handler)(void *));

void ConnectionPoolFree(struct ConnectionPool **);

void ConnectionPoolAccept(int sockfd);
