//
// Created by wieceslaw on 22.12.23.
//

#include <malloc.h>
#include "network.h"
#include "DatabaseWrapper.h"
#include "util_string.h"

struct DatabaseWrapper *DatabaseWrapperNew(char *filename, file_open_mode mode) {
    filename = string_copy(filename);
    if (filename == NULL) {
        return NULL;
    }
    struct DatabaseWrapper *wrapper = malloc(sizeof(struct DatabaseWrapper));
    if (wrapper == NULL) {
        return NULL;
    }
    wrapper->settings = (file_settings) {.path = filename, .open_mode = mode};
    if (pthread_mutex_init(&wrapper->lock, NULL) != 0) {
        free(wrapper);
        return NULL;
    }
    wrapper->db = DatabaseNew(&wrapper->settings);
    if (wrapper->db == NULL) {
        pthread_mutex_destroy(&wrapper->lock);
        free(wrapper);
        return NULL;
    }
    return wrapper;
}

int DatabaseWrapperFree(struct DatabaseWrapper *wrapper) {
    int result = 0;
    result |= pthread_mutex_destroy(&wrapper->lock);
    result |= DatabaseFree(wrapper->db);
    free(wrapper->settings.path);
    free(wrapper);
    return result;
}
