//
// Created by wieceslaw on 24.12.23.
//

#ifndef SINGLE_FILE_DATABASE_DATABASEWRAPPER_H
#define SINGLE_FILE_DATABASE_DATABASEWRAPPER_H

#include <pthread.h>
#include "database/Database.h"
#include "network.h"

struct DatabaseWrapper {
    file_settings settings;
    Database db;
    pthread_mutex_t lock;
};

struct DatabaseWrapper *DatabaseWrapperNew(char *filename, file_open_mode mode);

int DatabaseWrapperFree(struct DatabaseWrapper *);

Response *DatabaseWrapperExecute(struct DatabaseWrapper *wrapper, Request *request);

#endif //SINGLE_FILE_DATABASE_DATABASEWRAPPER_H
