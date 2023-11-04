file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)

add_library(ParserLib STATIC ${SOURCES})

target_include_directories(ParserLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
