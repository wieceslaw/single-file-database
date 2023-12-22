set(TARGET_NAME NetworkLib)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)
file(GLOB_RECURSE PROTO_FILES **.proto)

set(PROTO_HEADERS)
set(PROTO_SOURCES)
foreach (PROTO_FILE ${PROTO_FILES})
    string(REGEX MATCH "^.*/(.*).proto$" _ ${PROTO_FILE})
    set(PROTO_NAME ${CMAKE_MATCH_1})
    message("PROTO_NAME=${PROTO_NAME}")
    set(PROTO_SOURCES "${PROTO_SOURCES};${PROTO_NAME}.pb-c.c")
    set(PROTO_HEADERS "${PROTO_HEADERS};${PROTO_NAME}.pb-c.h")
endforeach ()

message("PROTO_HEADERS=${PROTO_HEADERS}")
message("PROTO_SOURCES=${PROTO_SOURCES}")
message("PROTO_FILES=${PROTO_FILES}")
message("SOURCES=${SOURCES}")

add_library(${TARGET_NAME} STATIC ${SOURCES} ${PROTO_SOURCES})

# proto generator begin
add_custom_target(ProtoGenerator DEPENDS ${PROTO_HEADERS} ${PROTO_SOURCES}
        COMMENT "Checking if re-generation is required")
add_custom_command(OUTPUT ${PROTO_HEADERS} ${PROTO_SOURCES}
        COMMAND protoc -I=${CMAKE_CURRENT_LIST_DIR}/proto --c_out=. ${PROTO_FILES}
        DEPENDS ${PROTO_FILES}
        COMMENT "Generating proto files")
add_dependencies(${TARGET_NAME} ProtoGenerator)
# proto generator end

target_include_directories(${TARGET_NAME} PUBLIC . ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${TARGET_NAME} protobuf-c)
target_link_libraries(${TARGET_NAME} UtilLib)
