//
// Created by wieceslaw on 22.12.23.
//

#include <malloc.h>
#include "network.h"
#include "DatabaseWrapper.h"
#include "util_string.h"

#define ERR__INVALID_REQUEST "INVALID REQUEST"
#define ERR__SERVER_ERROR "SERVER ERROR"
#define ERR__TABLE_ALREADY_EXISTS "TABLE ALREADY EXISTS"

static ColumnType ColumnTypeFromMsg(MsgColumnType columnType);

static MsgColumnType ColumnTypeToMsg(ColumnType columnType);

static MsgTableScheme *TableSchemeToMsg(table_scheme *scheme);

static SchemeBuilder TableSchemeFromMsg(MsgTableScheme *scheme);

static ListTableResponse *
ExecuteTablesList(struct DatabaseWrapper *wrapper, ListTableRequest *request, Response *response);

static CreateTableResponse *
ExecuteCreateTable(struct DatabaseWrapper *wrapper, CreateTableRequest *request, Response *response);

struct DatabaseWrapper *DatabaseWrapperNew(char *filename, file_open_mode mode) {
    filename = string_copy(filename);
    if (filename == NULL) {
        return NULL;
    }
    struct DatabaseWrapper *wrapper = malloc(sizeof(struct DatabaseWrapper));
    if (wrapper == NULL) {
        return NULL;
    }
    wrapper->settings = (file_settings){.path = filename, .open_mode = mode};
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

// TODO: many request-responses for select?
Response *DatabaseWrapperExecute(struct DatabaseWrapper *wrapper, Request *request) {
    pthread_mutex_lock(&wrapper->lock);
    Response *response = malloc(sizeof(Response));
    response__init(response);
    response->error = NULL;
    response->content_case = RESPONSE__CONTENT__NOT_SET;
    switch (request->content_case) {
        case REQUEST__CONTENT_TABLES_LIST: {
            ListTableRequest *ListTableRequest = request->tableslist;
            if (ListTableRequest == NULL) {
                response->error = ERR__INVALID_REQUEST;
                break;
            }
            ExecuteTablesList(wrapper, ListTableRequest, response);
            break;
        }
        case REQUEST__CONTENT_CREATE_TABLE: {
            CreateTableRequest *createTableRequest = request->createtable;
            if (createTableRequest == NULL) {
                response->error = ERR__INVALID_REQUEST;
                break;
            }
            ExecuteCreateTable(wrapper, createTableRequest, response);
            break;
        }
        default:
            response->error = ERR__INVALID_REQUEST;
    }
    pthread_mutex_unlock(&wrapper->lock);
    //    loginfo("RESPONSE STATUS: %s, MESSAGE: %s", response->ok ? "OK" : "ERROR", response->message);
    return response;
}

ListTableResponse *
ExecuteTablesList(struct DatabaseWrapper *wrapper, ListTableRequest *request, Response *response) {
    assert(wrapper != NULL && request != NULL);
    response->content_case = RESPONSE__CONTENT_TABLE_LIST;
    ListTableResponse *result = malloc(sizeof(ListTableResponse));
    if (result == NULL) {
        response->error = ERR__SERVER_ERROR;
        return NULL;
    }
    list_table_response__init(result);
    StrTableSchemeMap tables = DatabaseGetTablesSchemes(wrapper->db);
    size_t count = MAP_SIZE(tables);
    result->n_tables = count;
    result->tables = malloc(sizeof(MsgTableScheme *) * count);
    if (result->tables == NULL) {
        free(result);
        response->error = ERR__SERVER_ERROR;
        return NULL;
    }
    int i = 0;
    FOR_MAP(tables, entry, {
            result->tables[i] = TableSchemeToMsg(entry->val);
            free(entry->key);
            table_scheme_free(entry->val);
            i++;
            })
    return result;
}

// TODO: change return type to error message?
CreateTableResponse *
ExecuteCreateTable(struct DatabaseWrapper *wrapper, CreateTableRequest *request, Response *response) {
    assert(wrapper != NULL && request != NULL && request->scheme != NULL);
    response->content_case = RESPONSE__CONTENT_CREATE_TABLE;
    int err;
    SchemeBuilder scheme = TableSchemeFromMsg(request->scheme);
    if (scheme == NULL) {
        response->error = ERR__SERVER_ERROR;
        return NULL;
    }
    // TODO: differentiate between database fail and table already exists
    err = DatabaseCreateTable(wrapper->db, scheme);
    SchemeBuilderFree(scheme);
    if (err != 0) {
        response->error = ERR__TABLE_ALREADY_EXISTS;
        return NULL;
    }
    return NULL;
}

MsgTableScheme *TableSchemeToMsg(table_scheme *scheme) {
    assert(scheme != NULL);
    MsgTableScheme *result = malloc(sizeof(MsgTableScheme));
    msg_table_scheme__init(result);
    result->name = string_copy(scheme->name);
    result->n_columns = scheme->size;
    result->columns = malloc(sizeof(MsgColumnScheme *) * scheme->size);
    for (size_t i = 0; i < scheme->size; i++) {
        MsgColumnScheme *column = malloc(sizeof(MsgColumnScheme));
        msg_column_scheme__init(column);
        column->name = string_copy(scheme->columns[i].name);
        column->type = ColumnTypeToMsg(scheme->columns[i].type);
        result->columns[i] = column;
    }
    return result;
}

SchemeBuilder TableSchemeFromMsg(MsgTableScheme *scheme) {
    assert(scheme != NULL);
    SchemeBuilder schemeBuilder = SchemeBuilderNew(scheme->name);
    if (schemeBuilder == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < scheme->n_columns; i++) {
        MsgColumnScheme *column = scheme->columns[i];
        char *columnName = column->name;
        ColumnType columnType = ColumnTypeFromMsg(column->type);
        if (SchemeBuilderAddColumn(schemeBuilder, columnName, columnType) != 0) {
            SchemeBuilderFree(schemeBuilder);
            return NULL;
        }
    }
    return schemeBuilder;
}

MsgColumnType ColumnTypeToMsg(ColumnType columnType) {
    switch (columnType) {
        case COLUMN_TYPE_BOOL:
            return MSG_COLUMN_TYPE__BOOL;
        case COLUMN_TYPE_FLOAT:
            return MSG_COLUMN_TYPE__FLOAT32;
        case COLUMN_TYPE_INT:
            return MSG_COLUMN_TYPE__INT32;
        case COLUMN_TYPE_STRING:
            return MSG_COLUMN_TYPE__STRING;
        default:
            assert(0);
    }
}

ColumnType ColumnTypeFromMsg(MsgColumnType columnType) {
    switch (columnType) {
        case MSG_COLUMN_TYPE__INT32:
            return COLUMN_TYPE_INT;
        case MSG_COLUMN_TYPE__FLOAT32:
            return COLUMN_TYPE_FLOAT;
        case MSG_COLUMN_TYPE__STRING:
            return COLUMN_TYPE_STRING;
        case MSG_COLUMN_TYPE__BOOL:
            return COLUMN_TYPE_BOOL;
        default:
            assert(0);
    }
}
