//
// Created by wieceslaw on 18.12.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "lib.h"
#include "Server.h"

//struct threadInfo {
//    pthread_mutex_t *lock;
//    int sockfd;
//};
//
//void *handle(void *arg) {
//    struct threadInfo *info = arg;
//
//    printf("handle \n");
//
//    while (info->sockfd) {
//        pthread_mutex_lock(info->lock);
//        //
//        Message *message = receiveMessage(info->sockfd);
//        if (message == NULL) {
//            break;
//        }
//        char *str = message->request->message;
//        printf("%s", str);
//        message__free_unpacked(message, NULL);
//        //
//        pthread_mutex_unlock(info->lock);
//    }
//    close(info->sockfd);
//
//    return NULL;
//}
//
//int createThread(struct threadInfo *info) {
//    pthread_t *pthread = malloc(sizeof(pthread_t));
//    *pthread = (pthread_t) {0};
//    return pthread_create(pthread, NULL, &handle, info);
//    // TODO: how to free after malloc?
//}

int main(int argc, char *argv[]) {
    uint16_t port = 0;
    if (argc != 1 && argc != 3) {
        fprintf(stderr, "Wrong number of arguments, optional: -p <port> \n");
        return EXIT_FAILURE;
    }
    if (argc == 3) {
        if (strcmp(argv[1], "-p") != 0) {
            fprintf(stderr, "Unknown param: \"%s\" \n", argv[1]);
            return EXIT_FAILURE;
        }
        if (parsePort(argv[2], &port) != 0) {
            fprintf(stderr, "Unable to parse port \n");
            return EXIT_FAILURE;
        }
    }

    struct Server *server = ServerNew(port);

    pthread_t serveThread = {0};
    int err = pthread_create(&serveThread, NULL, &ServerStart, server);
    if (err != 0) {
        ServerFree(&server);
        perror("Server start failed");
        return EXIT_FAILURE;
    }
    printf("Server started using port: %d \n", server->port);

    char *line = NULL;
    size_t length = 0;
    while (getline(&line, &length, stdin) != -1) {
        printf("Command: %s", line);
        if (strcmp(line, "exit\n") == 0) {
            printf("Exiting... \n");
            break;
        }
    }
    free(line);

    ServerStop(server);
    ServerFree(&server);
    printf("Server stopped \n");
    return EXIT_SUCCESS;
}
