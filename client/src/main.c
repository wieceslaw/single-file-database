//
// Created by wieceslaw on 18.12.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#include "lib.h"

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


    Message message;
    message__init(&message);
    Request request;
    request__init(&request);
    message.request = &request;
    message.content_case = MESSAGE__CONTENT_REQUEST;
    request.data_case = REQUEST__DATA_MESSAGE;

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    while ((nread = getline(&line, &len, stdin)) != -1) {
        line[strlen(line) - 1] = '\0';
        request.message = line;
        if (sendMessage(sockfd, &message)) {
            printf("Sending response error \n");
            return EXIT_FAILURE;
        }
        Message *response = receiveMessage(sockfd);
        if (response == NULL) {
            printf("Receiving response error \n");
            return EXIT_FAILURE;
        }
        printf("Server response: \"%s\" \n", response->response->message);
        message__free_unpacked(response, NULL);
    }
    free(line);

    close(sockfd);
    return EXIT_SUCCESS;
}
