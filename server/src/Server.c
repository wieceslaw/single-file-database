//
// Created by wieceslaw on 22.12.23.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "Server.h"
#include "lib.h"

struct Server *ServerNew(uint16_t port) {
    struct Server *server = malloc(sizeof(struct Server));
    if (server == NULL) {
        return NULL;
    }
    *server = (struct Server) {0};
    server->serv.sin_family = AF_INET;
    server->serv.sin_addr.s_addr = INADDR_ANY;
    server->serv.sin_port = port;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        free(server);
        perror("Unable to create server socket");
        return NULL;
    }
    if (bind(sockfd, (struct sockaddr *) &(server->serv), sizeof(server->serv)) != 0) {
        free(server);
        perror("Unable to bind server socket");
        return NULL;
    }
    server->len = sizeof(server->serv);
    if (getsockname(sockfd, (struct sockaddr *) &(server->serv), &(server->len)) != 0) {
        free(server);
        perror("Unable to get server socket info");
        return NULL;
    }
    if (pthread_mutex_init(&(server->lock), NULL) != 0) {
        free(server);
        perror("Mutex init has failed \n");
        return NULL;
    }
    listen(sockfd, 1);
    server->port = ntohs(server->serv.sin_port);
    server->sockfd = sockfd;
    return server;
}

void ServerFree(struct Server **serverPtr) {
    if (serverPtr == NULL) {
        return;
    }
    struct Server *server = *serverPtr;
    close(server->sockfd);
    free(server);
    *serverPtr = NULL;
}

void *ServerStart(void *ptr) {
    struct Server* server = ptr;
    while (1) {
        struct sockaddr_in dest = {0};
        socklen_t socksize = sizeof(struct sockaddr_in);
        int confd = accept(server->sockfd, (struct sockaddr *) &dest, &socksize);
        if (confd < 0) {
            printf("Server accept failed \n");
            exit(1);
        }
        printf("[INFO]: New connection \n");
        Message message;
        message__init(&message);
        Response response;
        response__init(&response);
        message.response = &response;
        message.content_case = MESSAGE__CONTENT_RESPONSE;
        response.data_case = RESPONSE__DATA_MESSAGE;
        response.message = "Hello from server!";
        sendMessage(confd, &message);
    }
}

void ServerStop(struct Server *server) {
    // TODO: close all connections & cancel all threads
}
