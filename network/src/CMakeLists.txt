set(TARGET_NAME NetworkLib)

set(PROTO_PATH proto)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)
file(GLOB_RECURSE PROTO_FILES ${PROTO_PATH}/**.proto)
file(GLOB_RECURSE PROTO_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/**.pb-c.h)
file(GLOB_RECURSE PROTO_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/**.pb-c.c)
message("PROTO_HEADERS=${PROTO_HEADERS}")
message("PROTO_OUTPUTS=${PROTO_OUTPUTS}")

add_library(${TARGET_NAME} STATIC ${SOURCES} ${PROTO_OUTPUTS})

# proto generator begin
add_custom_target(ProtoGenerator DEPENDS ${PROTO_HEADERS} ${PROTO_OUTPUTS}
        COMMENT "Checking if re-generation is required")
add_custom_command(OUTPUT ${PROTO_HEADERS} ${PROTO_OUTPUTS}
        COMMAND protoc -I=${CMAKE_CURRENT_LIST_DIR}/proto --c_out=. ${PROTO_FILES}
        DEPENDS ${PROTO_FILES}
        COMMENT "Generating proto files")
add_dependencies(${TARGET_NAME} ProtoGenerator)
# proto generator end

target_include_directories(${TARGET_NAME} PUBLIC . ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${TARGET_NAME} protobuf-c)
target_link_libraries(${TARGET_NAME} UtilLib)
