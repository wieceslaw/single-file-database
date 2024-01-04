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

static file_open_mode OpenModeFromString(char *value) {
    if (strcmp(value, "create") == 0) {
        return FILE_OPEN_CREATE;
    } else if (strcmp(value, "exist") == 0) {
        return FILE_OPEN_EXIST;
    } else if (strcmp(value, "clear") == 0) {
        return FILE_OPEN_CLEAR;
    } else {
        printf("Unknown open mode");
        assert(0);
    }
}

int main(int argc, char *argv[]) {
    char *portValue = NULL;
    char *fileValue = NULL;
    char *modeValue = NULL;
    const char *short_options = "hp:f:m:";
    const struct option long_options[] = {
            {"help", no_argument,       NULL, 'h'},
            {"port", optional_argument, NULL, 'p'},
            {"file", required_argument, NULL, 'f'},
            {"mode", required_argument, NULL, 'm'},
            {NULL, 0,                   NULL, 0}
    };
    int rez;
    int option_index;
    while ((rez = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (rez) {
            case 'h': {
                printf(
                        "-p, --port - optional argument to specify port, otherwise - uses any available\n"
                        "-m, --mode [=create, exist, clear] - required argument for file open mode \n"
                        "-f, --file - required argument with file path \n"
                );
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
            case 'm': {
                modeValue = optarg;
                break;
            }
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
        loginfo("Port is not specified. Using available");
    } else {
        if (parsePort(portValue, &port) != 0) {
            fprintf(stderr, "Incorrect port value");
            return EXIT_FAILURE;
        }
    }


    struct Server *server = ServerNew(port, fileValue, OpenModeFromString(modeValue));
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
