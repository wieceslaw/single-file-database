file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)
add_executable(ParserTest ${SOURCES})
target_include_directories(ParserTest PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(ParserTest ParserLib)
