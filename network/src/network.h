//
// Created by wieceslaw on 18.12.23.
//

#ifndef SINGLE_FILE_DATABASE_NETWORK_H
#define SINGLE_FILE_DATABASE_NETWORK_H

#include <netinet/in.h>
#include "message.pb-c.h"

int parseIp4Address(char *str, struct in_addr *dst);

int parsePort(char *str, uint16_t *dst);

int sendMessage(int sockfd, Message *message);

Message *receiveMessage(int sockfd);

int sendResponse(int sockfd, Response *response);

int sendRequest(int sockfd, Request *request);

#endif //SINGLE_FILE_DATABASE_NETWORK_H
