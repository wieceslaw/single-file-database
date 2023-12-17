set(LIBNAME NetworkLib)

set(PROTO_PATH proto)
file(GLOB_RECURSE PROTO_FILES ${PROTO_PATH}/**.proto)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)

# proto sources begin
set(PROTO_HEADERS)
set(PROTO_SOURCES)
file(GLOB_RECURSE PROTO_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/**.pb-c.c)
foreach (PROTO_FILE ${PROTO_FILES})
    string(REGEX MATCH "^.*/(.*).proto$" _ ${PROTO_FILE})
    set(PROTO_NAME ${CMAKE_MATCH_1})
    message("PROTO_NAME=${PROTO_NAME}")
    set(PROTO_SRC "${PROTO_NAME}.pb-c.c")
    set(PROTO_HDR "${PROTO_NAME}.pb-c.h")
    set(PROTO_SOURCES "${PROTO_SOURCES} ${PROTO_SRC}")
    set(PROTO_HEADERS "${PROTO_HEADERS} ${PROTO_HDR}")
    string(REGEX MATCH "^.*/(proto.*).proto$" _ ${PROTO_FILE})
endforeach ()
message("PROTO_HEADERS=${PROTO_HEADERS}")
message("PROTO_SOURCES=${PROTO_SOURCES}")
# proto sources end

add_library(${LIBNAME} STATIC ${SOURCES} ${PROTO_OUTPUTS})

# proto generator begin
add_custom_target(ProtoGenerator DEPENDS ${PROTO_HEADERS} ${PROTO_OUTPUTS}
        COMMENT "Checking if re-generation is required")
add_custom_command(OUTPUT ${PROTO_HEADERS} ${PROTO_OUTPUTS}
        COMMAND protoc -I=${CMAKE_CURRENT_LIST_DIR} --c_out=. ${PROTO_FILES}
        DEPENDS ${PROTO_FILES}
        COMMENT "Generating proto files")
add_dependencies(${LIBNAME} ProtoGenerator)
# proto generator end

target_include_directories(${LIBNAME} PUBLIC . ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${LIBNAME} /usr/local/lib/libprotobuf-c.a) # staticly link with protobuf-c library
