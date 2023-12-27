//
// Created by wieceslaw on 22.12.23.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "Server.h"
#include "Connection.h"
#include "defines.h"

struct Server *ServerNew(uint16_t port, char *filename, file_open_mode mode) {
    struct Server *server = malloc(sizeof(struct Server));
    if (server == NULL) {
        return NULL;
    }
    *server = (struct Server){0};
    server->serv.sin_family = AF_INET;
    server->serv.sin_addr.s_addr = INADDR_ANY;
    server->serv.sin_port = port;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        free(server);
        logerr("Unable to create server socket");
        return NULL;
    }
    if (bind(sockfd, (struct sockaddr *) &server->serv, sizeof(server->serv)) != 0) {
        free(server);
        close(sockfd);
        logerr("Unable to bind server socket");
        return NULL;
    }
    server->len = sizeof(server->serv);
    if (getsockname(sockfd, (struct sockaddr *) &server->serv, &server->len) != 0) {
        free(server);
        close(sockfd);
        logerr("Unable to get server socket info");
        return NULL;
    }
    if (pthread_mutex_init(&server->lock, NULL) != 0) {
        free(server);
        close(sockfd);
        logerr("Mutex init has failed");
        return NULL;
    }
    struct DatabaseWrapper *databaseWrapper = DatabaseWrapperNew(filename, mode);
    if (databaseWrapper == NULL) {
        free(server);
        close(sockfd);
        pthread_mutex_destroy(&server->lock);
        logerr("Database init has failed");
        return NULL;
    }
    listen(sockfd, 1);
    server->databaseWrapper = databaseWrapper;
    server->port = ntohs(server->serv.sin_port);
    server->sockfd = sockfd;
    server->connections = ListNew();
    return server;
}

int ServerFree(struct Server *server) {
    if (server == NULL) {
        return 0;
    }
    int err = 0;
    err |= close(server->sockfd);
    err |= pthread_cancel(server->loopThread);
    ListIterator it = ListHeadIterator(server->connections);
    while (!ListIteratorIsEmpty(it)) {
        struct Connection *connection = ListIteratorGet(it);
        err |= ConnectionStop(connection);
        ConnectionFree(connection);
        ListIteratorNext(it);
    }
    ListIteratorFree(&(it));
    ListFree(server->connections);
    server->connections = NULL;
    err |= pthread_mutex_destroy(&server->lock);
    err |= DatabaseWrapperFree(server->databaseWrapper);
    free(server);
    return err;
}

static int ServerAcceptConnection(struct Server *server) {
    struct Connection *connection = ConnectionNew(server);
    if (connection == NULL) {
        return -1;
    }
    connection->sockfd = accept(server->sockfd, (struct sockaddr *) &(connection->dest), &(connection->socksize));
    if (connection->sockfd < 0) {
        logerr("Server accept failed");
        ConnectionFree(connection);
        return -1;
    }
    pthread_mutex_lock(&server->lock);
    ListAppendTail(server->connections, connection);
    connection->node = ListTailIterator(server->connections);
    pthread_mutex_unlock(&server->lock);
    if (ConnectionStart(connection) != 0) {
        ConnectionFree(connection);
        return -1;
    }
    return connection->sockfd;
}

static void *ServerLoop(struct Server *server) {
    while (1) {
        int sockfd = ServerAcceptConnection(server);
        if (sockfd == -1) {
            logerr("Connection accept error");
            return NULL;
        }
        loginfo("New client");
    }
}

int ServerStart(struct Server *server) {
    int err = pthread_create(&(server->loopThread), NULL, (void *(*)(void *)) &ServerLoop, server);
    if (err != 0) {
        logerr("Server accept thread start failed");
        return err;
    }
    return 0;
}
