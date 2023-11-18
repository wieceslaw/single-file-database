file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)

add_library(UtilLib STATIC ${SOURCES})

target_include_directories(UtilLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
