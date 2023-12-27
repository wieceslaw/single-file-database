//
// Created by wieceslaw on 18.12.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "network.h"
#include "Server.h"
#include "defines.h"

int main(int argc, char *argv[]) {
    char *portValue = 0;
    char *fileValue = 0;
    const char *short_options = "hp:f:";
    const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"port", optional_argument, NULL, 'p'},
        {"file", required_argument, NULL, 'f'},
        {NULL, 0, NULL, 0}
    };
    int rez;
    int option_index;
    while ((rez = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (rez) {
            case 'h': {
                printf("This is demo help. Try -h or --help.\n");
                return EXIT_SUCCESS;
            };
            case 'p': {
                portValue = optarg;
                break;
            };
            case 'f': {
                fileValue = optarg;
                break;
            };
            case '?':
            default: {
                fprintf(stderr, "Unknown option: %s", optarg);
                return EXIT_FAILURE;
            };
        };
    };
    if (fileValue == NULL) {
        fprintf(stderr, "File must be specified. Use --file or -f");
        return EXIT_FAILURE;
    }

    uint16_t port = 0;
    if (portValue == NULL) {
        loginfo("Port if not specified. Using available");
    } else {
        if (parsePort(portValue, &port) != 0) {
            fprintf(stderr, "Incorrect port value");
            return EXIT_FAILURE;
        }
    }

    struct Server *server = ServerNew(port, fileValue, FILE_OPEN_EXIST);
    if (ServerStart(server) != 0) {
        ServerFree(server);
        return EXIT_FAILURE;
    }
    loginfo("Server started with port: %d", server->port);

    char *line = NULL;
    size_t length = 0;
    while (getline(&line, &length, stdin) != -1) {
        line[strlen(line) - 1] = '\0';
        if (strcmp(line, "stop") == 0) {
            loginfo("Stopping server...");
            break;
        } else {
            loginfo("Unknown command: %s", line);
        }
    }
    free(line);

    ServerFree(server);
    loginfo("Server stopped");
    return EXIT_SUCCESS;
}
