//
// Created by wieceslaw on 22.12.23.
//

#include <malloc.h>
#include <unistd.h>
#include "Connection.h"
#include "defines.h"

static int handle(struct Connection *connection) {
    while (connection->sockfd) {
        Message *message = receiveMessage(connection->sockfd);
        if (message == NULL) {
            return 0;
        }
        if (message->content_case != MESSAGE__CONTENT_REQUEST) {
            message__free_unpacked(message, NULL);
            debug("Wrong request type");
            return -1;
        }
        Request *request = message->request;
        if (request == NULL) {
            message__free_unpacked(message, NULL);
            debug("Error: empty request");
            return -1;
        }
        Response *response = DatabaseWrapperExecute(connection->server->databaseWrapper, request);
        message__free_unpacked(message, NULL);
        if (response == NULL) {
            debug("Database execution error");
            return -1;
        }
        sendResponse(connection->sockfd, response);
    }
    return 0;
}

static void *ConnectionRun(struct Connection *connection) {
    debug("Connection run");
    int err = handle(connection);
    if (err != 0) {
        logerr("Connection handling error");
    } else {
        loginfo("Client disconnect");
    }
    ConnectionFree(connection);
    return NULL;
}

struct Connection *ConnectionNew(struct Server *server) {
    struct Connection *connection = malloc(sizeof(struct Connection));
    if (connection == NULL) {
        return NULL;
    }
    *connection = (struct Connection) {0};
    connection->dest = (struct sockaddr_in) {0};
    connection->socksize = sizeof(struct sockaddr_in);
    connection->sockfd = 0;
    connection->server = server;
    return connection;
}

int ConnectionStart(struct Connection *connection) {
    int err = pthread_create(&(connection->thread), NULL, (void *(*)(void *)) &ConnectionRun, connection);
    if (err != 0) {
        logerr("Server accept thread start failed");
        return -1;
    }
    return 0;
}

void ConnectionFree(struct Connection *connection) {
    if (connection == NULL) {
        return;
    }
    close(connection->sockfd);
    pthread_mutex_lock(&connection->server->lock);
    ListIteratorDeleteNode(connection->node);
    pthread_mutex_unlock(&connection->server->lock);
    free(connection);
    debug("Connection free");
}

int ConnectionStop(struct Connection *connection) {
    return pthread_cancel(connection->thread);
}
