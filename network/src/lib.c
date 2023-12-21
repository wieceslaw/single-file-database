//
// Created by wieceslaw on 18.12.23.
//

#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"

#define PORTMAX 65535
#define PORTMIN 1

int parseIp4Address(char *str, struct in_addr *dst) {
    assert(str != NULL && dst != NULL);
    if (!inet_aton(str, dst)) {
        fprintf(stderr, "Unable to parse address \n");
        return -1;
    }
    return 0;
}

int parsePort(char *str, uint16_t *dst) {
    assert(str != NULL && dst != NULL);
    char *end = 0;
    long port = strtol(str, &end, 10);
    if (end != str + strlen(str)) {
        fprintf(stderr, "Unable to parse port \n");
        return -1;
    }
    if (port > PORTMAX || port < PORTMIN) {
        fprintf(stderr, "Port number out of bounds \n");
        return -1;
    }
    *dst = htons(port);
    return 0;
}

int sendMessage(int sockfd, Message *message) {
    uint8_t pad[128];
    ProtobufCBufferSimple simple = PROTOBUF_C_BUFFER_SIMPLE_INIT(pad);
    ProtobufCBuffer *buffer = (ProtobufCBuffer *) &simple;
    size_t size = message__pack_to_buffer(message, buffer);
    ssize_t len;
    len = send(sockfd, &size, sizeof(size), 0);
    if (len != sizeof(size)) {
        PROTOBUF_C_BUFFER_SIMPLE_CLEAR(&simple);
        return -1;
    }
    len = send(sockfd, simple.data, size, 0);
    if (len != size) {
        PROTOBUF_C_BUFFER_SIMPLE_CLEAR(&simple);
        return -1;
    }
    PROTOBUF_C_BUFFER_SIMPLE_CLEAR(&simple);
    return 0;
}

Message *receiveMessage(int sockfd) {
    uint64_t size = 0;
    ssize_t len;
    len = recv(sockfd, &size, sizeof(size), 0);
    if (len != sizeof(size)) {
        return NULL;
    }
    uint8_t* data = malloc(size);
    if (data == NULL) {
        return NULL;
    }
    len = recv(sockfd, data, size, 0);
    if (len != size) {
        free(data);
        return NULL;
    }
    Message *message = message__unpack(NULL, size, data);
    free(data);
    return message;
}
