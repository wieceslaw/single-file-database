file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)

#----------
#find_package(Protobuf REQUIRED)
#include_directories(${Protobuf_INCLUDE_DIRS})
#include_directories(${CMAKE_CURRENT_BINARY_DIR})
#find_package(PkgConfig)
#pkg_search_module(PROTOBUF-C REQUIRED libprotobuf-c)
#----------

add_executable(Client ${SOURCES})
target_include_directories(Client PUBLIC ${CMAKE_CURRENT_LIST_DIR})
