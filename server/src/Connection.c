//
// Created by wieceslaw on 22.12.23.
//

#include <malloc.h>
#include <unistd.h>
#include "Connection.h"
#include "defines.h"
#include "util_string.h"

#define ERR__INTERNAL_ERROR "INTERNAl ERROR"
#define ERR__TABLE_ALREADY_EXISTS "TABLE ALREADY EXISTS"
#define ERR__UNKNOWN_TABLE "UNKNOWN TABLE"
#define ERR__INVALID_QUERY "INVALID QUERY"
#define ERR__BATCH_CORRUPTED "INAPPROPRIATE ROW VALUES"

static int
ConnectionHandleListTables(struct Connection *connection, struct DatabaseWrapper *wrapper, ListTableRequest *request);

static int ConnectionHandleCreateTable(struct Connection *connection, struct DatabaseWrapper *wrapper,
                                       CreateTableRequest *request);

static int ConnectionHandleDeleteTable(struct Connection *connection, struct DatabaseWrapper *wrapper,
                                       DeleteTableRequest *request);

static int
ConnectionHandleInsertTable(struct Connection *connection, struct DatabaseWrapper *wrapper, InsertRequest *request);

static int
ConnectionHandleUpdate(struct Connection *connection, struct DatabaseWrapper *wrapper, UpdateRequest *request);

static int
ConnectionHandleDelete(struct Connection *connection, struct DatabaseWrapper *wrapper, DeleteRequest *request);

static int
ConnectionHandleSelect(struct Connection *connection, struct DatabaseWrapper *wrapper, SelectRequest *request);

static ColumnType ColumnTypeFromMsg(MsgColumnType columnType);

static MsgColumnType ColumnTypeToMsg(ColumnType columnType);

static MsgTableScheme *TableSchemeToMsg(table_scheme *scheme);

static SchemeBuilder TableSchemeFromMsg(MsgTableScheme *scheme);

static int RowFromMsg(MsgRowData *mrow, Row *row);

static int ColumnFromMsg(MsgColumnData *mcolumn, Column *column);

static MsgRowData *RowToMsg(Row row);

static MsgColumnData *ColumnToMsg(Column column);

static Response *UpdateResponseNew(void);

int ConnectionSendResponse(struct Connection *connection, Response* response) {
    return sendResponse(connection->sockfd, response);
}

int ConnectionSendError(struct Connection *connection, char* message) {
    Response response;
    response__init(&response);
    response.content_case = RESPONSE__CONTENT_INSERT;
    response.error = message;
    return ConnectionSendResponse(connection, &response);
}

static int handle(struct Connection *connection) {
    while (connection->sockfd) {
        Message *message = receiveMessage(connection->sockfd);
        if (message == NULL) {
            return 0;
        }
        if (message->content_case != MESSAGE__CONTENT_REQUEST) {
            message__free_unpacked(message, NULL);
            debug("Wrong request type");
            return -1;
        }
        Request *request = message->request;
        if (request == NULL) {
            message__free_unpacked(message, NULL);
            debug("Error: empty request");
            return -1;
        }
        int res;
        struct DatabaseWrapper *wrapper = connection->server->databaseWrapper;
        pthread_mutex_lock(&wrapper->lock);
        switch (request->content_case) {
            case REQUEST__CONTENT_TABLES_LIST:
                res = ConnectionHandleListTables(connection, wrapper, request->tableslist);
                break;
            case REQUEST__CONTENT_CREATE_TABLE:
                res = ConnectionHandleCreateTable(connection, wrapper, request->createtable);
                break;
            case REQUEST__CONTENT_DELETE_TABLE:
                res = ConnectionHandleDeleteTable(connection, wrapper, request->deletetable);
                break;
            case REQUEST__CONTENT_INSERT:
                res = ConnectionHandleInsertTable(connection, wrapper, request->insert);
                break;
            case REQUEST__CONTENT_DELETE:
                res = ConnectionHandleDelete(connection, wrapper, request->delete_);
                break;
            case REQUEST__CONTENT_UPDATE:
                res = ConnectionHandleUpdate(connection, wrapper, request->update);
                break;
            case REQUEST__CONTENT_SELECT:
                res = ConnectionHandleSelect(connection, wrapper, request->select);
                break;
            default:
                debug("Unknown request type");
                res = -1;
        }
        pthread_mutex_unlock(&wrapper->lock);
        message__free_unpacked(message, NULL);
        if (res != 0) {
            debug("Error while handling request");
            return -1;
        }
    }
    return 0;
}

static void *ConnectionRun(struct Connection *connection) {
    debug("Connection run");
    int err = handle(connection);
    if (err != 0) {
        logerr("Connection handling error");
    } else {
        loginfo("Client disconnect");
    }
    ConnectionFree(connection);
    return NULL;
}

struct Connection *ConnectionNew(struct Server *server) {
    struct Connection *connection = malloc(sizeof(struct Connection));
    if (connection == NULL) {
        return NULL;
    }
    *connection = (struct Connection) {0};
    connection->dest = (struct sockaddr_in) {0};
    connection->socksize = sizeof(struct sockaddr_in);
    connection->sockfd = 0;
    connection->server = server;
    return connection;
}

int ConnectionStart(struct Connection *connection) {
    int err = pthread_create(&(connection->thread), NULL, (void *(*)(void *)) &ConnectionRun, connection);
    if (err != 0) {
        logerr("Server accept thread start failed");
        return -1;
    }
    return 0;
}

void ConnectionFree(struct Connection *connection) {
    if (connection == NULL) {
        return;
    }
    pthread_mutex_lock(&connection->server->databaseWrapper->lock);
    close(connection->sockfd);
    pthread_mutex_unlock(&connection->server->databaseWrapper->lock);
    pthread_mutex_lock(&connection->server->lock);
    ListIteratorDeleteNode(connection->node);
    pthread_mutex_unlock(&connection->server->lock);
    free(connection);
    debug("Connection free");
}

int ConnectionStop(struct Connection *connection) {
    return pthread_cancel(connection->thread);
}

static Response *ListTablesResponseNew(void) {
    ListTableResponse *listTable = malloc(sizeof(ListTableResponse));
    list_table_response__init(listTable);
    Response *response = malloc(sizeof(Response));
    response__init(response);
    response->content_case = RESPONSE__CONTENT_TABLE_LIST;
    response->tablelist = listTable;
    return response;
}

static int
ConnectionHandleListTables(struct Connection *connection, struct DatabaseWrapper *wrapper, ListTableRequest *request) {
    assert(connection != NULL && request != NULL);
    StrTableSchemeMap tables = DatabaseGetTablesSchemes(wrapper->db);
    size_t count = MAP_SIZE(tables);
    Response *response = ListTablesResponseNew();
    ListTableResponse *listTables = response->tablelist;
    listTables->n_tables = count;
    listTables->tables = malloc(sizeof(MsgTableScheme *) * count);
    int i = 0;
    FOR_MAP(tables, entry, {
        listTables->tables[i] = TableSchemeToMsg(entry->val);
        free(entry->key);
        table_scheme_free(entry->val);
        i++;
    })
    int err = ConnectionSendResponse(connection, response);
    response__free_unpacked(response, NULL);
    return err;
}

static int ConnectionHandleCreateTable(struct Connection *connection, struct DatabaseWrapper *wrapper,
                                       CreateTableRequest *request) {
    assert(connection != NULL && request != NULL && request->scheme != NULL);
    CreateTableResponse result;
    create_table_response__init(&result);
    Response response;
    response__init(&response);
    response.content_case = RESPONSE__CONTENT_CREATE_TABLE;
    response.createtable = &result;
    SchemeBuilder scheme = TableSchemeFromMsg(request->scheme);
    if (scheme == NULL) {
        response.error = ERR__INTERNAL_ERROR;
        sendResponse(connection->sockfd, &response);
        return -1;
    }
    DatabaseResult res = DatabaseCreateTable(wrapper->db, scheme);
    SchemeBuilderFree(scheme);
    if (res != DB_OK) {
        int err;
        switch (res) {
            case DB_TABLE_EXISTS: {
                err = ConnectionSendError(connection, ERR__TABLE_ALREADY_EXISTS);
                break;
            }
            default: {
                err = ConnectionSendError(connection, ERR__INTERNAL_ERROR);
                break;
            }
        }
        if (res == DB_ERR) {
            err = -1;
        }
        return err;
    }
    return sendResponse(connection->sockfd, &response);
}

static int ConnectionHandleDeleteTable(struct Connection *connection, struct DatabaseWrapper *wrapper,
                                       DeleteTableRequest *request) {
    assert(connection != NULL && wrapper != NULL && request != NULL);
    DeleteTableResponse result;
    delete_table_response__init(&result);
    Response response;
    response__init(&response);
    response.content_case = RESPONSE__CONTENT_DELETE_TABLE;
    response.deletetable = &result;
    DatabaseResult res = DatabaseDeleteTable(wrapper->db, request->name);
    if (res != DB_OK) {
        switch (res) {
            case DB_UNKNOWN_TABLE: {
                response.error = ERR__UNKNOWN_TABLE;
                break;
            }
            default: {
                response.error = ERR__INTERNAL_ERROR;
                break;
            }
        }
        int err = sendResponse(connection->sockfd, &response);
        if (res == DB_ERR) {
            err = -1;
        }
        return err;
    }
    return sendResponse(connection->sockfd, &response);
}

static int
ConnectionHandleInsertTable(struct Connection *connection, struct DatabaseWrapper *wrapper, InsertRequest *request) {
    assert(connection != NULL && wrapper != NULL && request != NULL);
    InsertResponse result;
    insert_response__init(&result);
    Response response;
    response__init(&response);
    response.content_case = RESPONSE__CONTENT_INSERT;
    response.insert = &result;

    RowBatch batch = RowBatchNew(request->n_values);
    for (size_t i = 0; i < request->n_values; i++) {
        MsgRowData *mrow = request->values[i];
        Row row = {0};
        if (RowFromMsg(mrow, &row) != 0) {
            RowBatchFree(&batch);
            response.error = ERR__INTERNAL_ERROR;
            sendResponse(connection->sockfd, &response);
            return -1;
        }
        RowBatchAddRow(&batch, row);
    }
    DatabaseResult res = DatabaseInsertQuery(wrapper->db, request->table, batch);
    if (res != DB_OK) {
        switch (res) {
            case DB_UNKNOWN_TABLE: {
                response.error = ERR__UNKNOWN_TABLE;
                break;
            }
            case DB_BATCH_CORRUPTED: {
                response.error = ERR__BATCH_CORRUPTED;
                break;
            }
            default: {
                response.error = ERR__INTERNAL_ERROR;
                break;
            }
        }
        RowBatchFree(&batch);
        int err = sendResponse(connection->sockfd, &response);
        if (res == DB_ERR) {
            err = -1;
        }
        return err;
    }
    RowBatchFree(&batch);
    return sendResponse(connection->sockfd, &response);
}

static SelectorBuilder SelectorFromMsg(Selector *select) {
    SelectorBuilder builder = SelectorBuilderNew();
    for (size_t i = 0; i < select->n_columns; i++) {
        SelectorBuilderAdd(builder,
                           string_copy(select->columns[i]->table),
                           string_copy(select->columns[i]->column));
    }
    return builder;
}

static JoinBuilder JoinFromMsg(size_t n_joins, JoinCondition **joins) {
    JoinBuilder builder = JoinBuilderNew();
    for (size_t i = 0; i < n_joins; i++) {
        JoinBuilderAddCondition(
                builder,
                table_column_of(
                        joins[i]->left->table,
                        joins[i]->left->column
                ),
                table_column_of(
                        joins[i]->right->table,
                        joins[i]->right->column
                )
        );
    }
    return builder;
}

static operand PredicateOperandFromMsg(MsgPredicateOperand *moperand) {
    switch (moperand->content_case) {
        case MSG_PREDICATE_OPERAND__CONTENT_LITERAL: {
            Column column;
            ColumnFromMsg(moperand->literal, &column);
            return (operand) {
                    .type = OPERAND_VALUE_LITERAL,
                    .literal = column
            };
        }
        case MSG_PREDICATE_OPERAND__CONTENT_COLUMN: {
            operand op;
            op.type = OPERAND_VALUE_COLUMN;
            op.column.type = COLUMN_DESC_NAME;
            op.column.name.column_name = string_copy(moperand->column->column);
            op.column.name.table_name = string_copy(moperand->column->table);
            return op;
        }
        default: {
            debug("Unexpected operand type");
            assert(0);
        }
    }
}

static comparing_type CompareTypeFromMsg(MsgPredicateCompareType type) {
    switch (type) {
        case MSG_PREDICATE_COMPARE_TYPE__CMP_LE:
            return COMPARE_LE;
        case MSG_PREDICATE_COMPARE_TYPE__CMP_GE:
            return COMPARE_GE;
        case MSG_PREDICATE_COMPARE_TYPE__CMP_LS:
            return COMPARE_LT;
        case MSG_PREDICATE_COMPARE_TYPE__CMP_GR:
            return COMPARE_GT;
        case MSG_PREDICATE_COMPARE_TYPE__CMP_EQ:
            return COMPARE_EQ;
        case MSG_PREDICATE_COMPARE_TYPE__CMP_NQ:
            return COMPARE_NE;
        default:
            debug("Unknown compare type");
            assert(0);
    }
}

static where_condition* WhereFromMsg(MsgPredicate *predicate) {
    if (predicate == NULL) {
        return NULL;
    }
    where_condition *condition = malloc(sizeof(where_condition));
    switch (predicate->content_case) {
        case MSG_PREDICATE__CONTENT_COMPARE: {
            condition->type = CONDITION_COMPARE;
            condition->compare.type = CompareTypeFromMsg(predicate->compare->type);
            condition->compare.first = PredicateOperandFromMsg(predicate->compare->left);
            condition->compare.second = PredicateOperandFromMsg(predicate->compare->right);
            break;
        }
        case MSG_PREDICATE__CONTENT_AND: {
            condition->type = CONDITION_AND;
            condition->and.first = WhereFromMsg(predicate->and_->first);
            condition->and.second = WhereFromMsg(predicate->and_->second);
            break;
        }
        case MSG_PREDICATE__CONTENT_OR: {
            condition->type = CONDITION_OR;
            condition->or.first = WhereFromMsg(predicate->or_->first);
            condition->or.second = WhereFromMsg(predicate->or_->second);
            break;
        }
        case MSG_PREDICATE__CONTENT_NOT: {
            condition->type = CONDITION_NOT;
            condition->not.first = WhereFromMsg(predicate->not_->first);
            break;
        }
        default: {
            debug("Unexpected predicate type");
            return NULL;
        }
    }
    return condition;
}

static Response *SelectResponseNew(void) {
    SelectResponse *select = malloc(sizeof(SelectResponse));
    select_response__init(select);
    Response *response = malloc(sizeof(Response));
    response__init(response);
    response->content_case = RESPONSE__CONTENT_SELECT;
    response->select = select;
    return response;
}

static MsgColumnData *ColumnToMsg(Column column) {
    MsgColumnData *mcolumn = malloc(sizeof(MsgColumnData));
    msg_column_data__init(mcolumn);
    switch (column.type) {
        case COLUMN_TYPE_INT32:
            mcolumn->value_case = MSG_COLUMN_DATA__VALUE_I;
            mcolumn->i = column.value.i32;
            break;
        case COLUMN_TYPE_FLOAT32:
            mcolumn->value_case = MSG_COLUMN_DATA__VALUE_F;
            mcolumn->f = column.value.f32;
            break;
        case COLUMN_TYPE_STRING:
            mcolumn->value_case = MSG_COLUMN_DATA__VALUE_S;
            mcolumn->s = string_copy(column.value.str);
            break;
        case COLUMN_TYPE_BOOL:
            mcolumn->value_case = MSG_COLUMN_DATA__VALUE_B;
            mcolumn->b = column.value.b8;
            break;
        default:
            debug("Unexpected column type");
            assert(0);
    }
    return mcolumn;
}

static MsgRowData *RowToMsg(Row row) {
    MsgRowData *mrow = malloc(sizeof(MsgRowData));
    msg_row_data__init(mrow);
    mrow->n_columns = row.size;
    mrow->columns = malloc(sizeof(MsgColumnData) * row.size);
    for (size_t i = 0; i < row.size; i++) {
        mrow->columns[i] = ColumnToMsg(row.columns[i]);
    }
    return mrow;
}

static int sendRow(int sockfd, Row row) {
    RowBatchResponse batch;
    row_batch_response__init(&batch);
    Response response;
    response__init(&response);
    response.content_case = RESPONSE__CONTENT_BATCH;
    response.batch = &batch;
    MsgRowData *mrow = RowToMsg(row);
    batch.data = mrow;
    int err = sendResponse(sockfd, &response);
    msg_row_data__free_unpacked(mrow, NULL);
    return err;
}

static int sendBatchEnd(int sockfd, int count) {
    RowBatchEndResponse batchEnd;
    row_batch_end_response__init(&batchEnd);
    batchEnd.count = count;
    Response response;
    response__init(&response);
    response.batchend = &batchEnd;
    response.content_case = RESPONSE__CONTENT_BATCH_END;
    return sendResponse(sockfd, &response);
}

static int
ConnectionHandleSelect(struct Connection *connection, struct DatabaseWrapper *wrapper, SelectRequest *request) {
    assert(connection != NULL && wrapper != NULL && request != NULL);
    SelectorBuilder selector = SelectorFromMsg(request->selector);
    where_condition *where = WhereFromMsg(request->where);
    JoinBuilder join = JoinFromMsg(request->n_joins, request->joins);
    query_t query = {.table = request->table, .where = where, .joins = join};
    ResultView view;
    DatabaseResult result = DatabaseSelectQuery(wrapper->db, query, selector, &view);
    if (result != DB_OK) {
        where_condition_free(where);
        JoinBuilderFree(join);
        SelectorBuilderFree(selector);
        int err = 0;
        switch (result) {
            case DB_INVALID_QUERY:
                ConnectionSendError(connection, ERR__INVALID_QUERY);
                break;
            default:
                err = -1;
                ConnectionSendError(connection, ERR__INTERNAL_ERROR);
                break;
        }
        return err;
    }
    Response *response = SelectResponseNew();
    response->select->scheme = TableSchemeToMsg(ResultViewGetScheme(view));
    int err = sendResponse(connection->sockfd, response);
    response__free_unpacked(response, NULL);

    int count = 0;
    while (!ResultViewIsEmpty(view)) {
        Row row = ResultViewGetRow(view);
        err = sendRow(connection->sockfd, row);
        if (err != 0) {
            break;
        }
        RowFree(row);
        ResultViewNext(view);
        count++;
    }
    sendBatchEnd(connection->sockfd, count);

    ResultViewFree(view);
    where_condition_free(where);
    JoinBuilderFree(join);
    SelectorBuilderFree(selector);
    return err;
}

static Response *DeleteResponseNew(void) {
    DeleteResponse *delete = malloc(sizeof(DeleteResponse));
    delete_response__init(delete);

    Response *response = malloc(sizeof(Response));
    response__init(response);
    response->content_case = RESPONSE__CONTENT_DELETE;
    response->delete_ = delete;

    return response;
}

static int
ConnectionHandleDelete(struct Connection *connection, struct DatabaseWrapper *wrapper, DeleteRequest *request) {
    assert(connection != NULL && wrapper != NULL && request != NULL);
    where_condition *where = WhereFromMsg(request->where);
    query_t query = {.table = request->table, .where = where, .joins = NULL};
    int count;
    DatabaseResult result = DatabaseDeleteQuery(wrapper->db, query, &count);
    where_condition_free(where);
    if (result != DB_OK) {
        switch (result) {
            case DB_INVALID_QUERY:
                ConnectionSendError(connection, ERR__INVALID_QUERY);
                return 0;
            default:
                ConnectionSendError(connection, ERR__INTERNAL_ERROR);
                return -1;
        }
    }
    Response *response = DeleteResponseNew();
    response->delete_->count = count;
    int err = sendResponse(connection->sockfd, response);;
    response__free_unpacked(response, NULL);
    return err;
}

static Response *UpdateResponseNew(void) {
    UpdateResponse *update = malloc(sizeof(UpdateResponse));
    update_response__init(update);

    Response *response = malloc(sizeof(Response));
    response__init(response);
    response->content_case = RESPONSE__CONTENT_UPDATE;
    response->update = update;

    return response;
}

static updater_builder_t UpdaterFromMsg(UpdateRequest *request) {
    updater_builder_t updater = updater_builder_init();
    for (size_t i = 0; i < request->n_sets; i++) {
        Column column;
        ColumnFromMsg(request->sets[i]->value, &column);
        updater_builder_add(updater,  column_updater_of(
                request->sets[i]->column->column,
                column
        ));
        ColumnFree(column);
    }
    return updater;
}

static int
ConnectionHandleUpdate(struct Connection *connection, struct DatabaseWrapper *wrapper, UpdateRequest *request) {
    assert(connection != NULL && wrapper != NULL && request != NULL);
    where_condition *where = WhereFromMsg(request->where);
    query_t query = {.table = request->table, .where = where, .joins = NULL};
    int count;
    updater_builder_t updater = UpdaterFromMsg(request);
    DatabaseResult result = DatabaseUpdateQuery(wrapper->db, query, updater, &count);
    where_condition_free(where);
    updater_builder_free(&updater);

    if (result != DB_OK) {
        switch (result) {
            case DB_INVALID_QUERY:
                ConnectionSendError(connection, ERR__INVALID_QUERY);
                return 0;
            default:
                ConnectionSendError(connection, ERR__INTERNAL_ERROR);
                return -1;
        }
    }
    Response *response = UpdateResponseNew();
    response->update->count = count;
    int err = sendResponse(connection->sockfd, response);;
    response__free_unpacked(response, NULL);
    return err;
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
        case COLUMN_TYPE_FLOAT32:
            return MSG_COLUMN_TYPE__FLOAT32;
        case COLUMN_TYPE_INT32:
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
            return COLUMN_TYPE_INT32;
        case MSG_COLUMN_TYPE__FLOAT32:
            return COLUMN_TYPE_FLOAT32;
        case MSG_COLUMN_TYPE__STRING:
            return COLUMN_TYPE_STRING;
        case MSG_COLUMN_TYPE__BOOL:
            return COLUMN_TYPE_BOOL;
        default:
            assert(0);
    }
}

static int ColumnFromMsg(MsgColumnData *mcolumn, Column *column) {
    switch (mcolumn->value_case) {
        case MSG_COLUMN_DATA__VALUE_I:
            column->type = COLUMN_TYPE_INT32;
            column->value.i32 = mcolumn->i;
            break;
        case MSG_COLUMN_DATA__VALUE_F:
            column->type = COLUMN_TYPE_FLOAT32;
            column->value.f32 = mcolumn->f;
            break;
        case MSG_COLUMN_DATA__VALUE_S:
            column->type = COLUMN_TYPE_STRING;
            column->value.str = string_copy(mcolumn->s);
            break;
        case MSG_COLUMN_DATA__VALUE_B:
            column->type = COLUMN_TYPE_BOOL;
            column->value.b8 = mcolumn->b;
            break;
        default:
            debug("Unknown column type");
            return -1;
    }
    return 0;
}

static int RowFromMsg(MsgRowData *mrow, Row *row) {
    row->size = mrow->n_columns;
    row->columns = malloc(sizeof(Column) * row->size);
    if (row->columns == NULL) {
        return -1;
    }
    for (size_t i = 0; i < mrow->n_columns; i++) {
        MsgColumnData *column = mrow->columns[i];
        if (ColumnFromMsg(column, &(row->columns[i])) != 0) {
            // free string columns?
            free(row->columns);
            return -1;
        }
    }
    return 0;
}
