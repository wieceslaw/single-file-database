//
// Created by vyach on 19.11.2023.
//

#include <stdio.h>
#include "lib.h"

int main() {
    Request request;
    request__init(&request);
//    DeleteTableQuery query;
//    delete_table_query__init(&query);
//    query.name = "Hello";
    request.type = REQUEST_TYPE__DELETE_QUERY;
    request.data_case = REQUEST__DATA_MESSAGE;
    request.message = "Hello";
    uint8_t pad[128];
    ProtobufCBufferSimple simple = PROTOBUF_C_BUFFER_SIMPLE_INIT(pad);
    ProtobufCBuffer *buffer = (ProtobufCBuffer *) &simple;
    size_t size = request__pack_to_buffer(&request, buffer);

    printf("Hello, world \n");
    return 0;
}
