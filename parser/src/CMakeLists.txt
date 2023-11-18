# ==== parsing ====
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
FLEX_TARGET(MyScanner lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ==== creating library =====
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)
add_library(ParserLib STATIC
        ${SOURCES}
        ${BISON_MyParser_OUTPUTS}
        ${FLEX_MyScanner_OUTPUTS}
)

target_link_libraries(ParserLib ${FLEX_LIBRARIES})
target_link_libraries(ParserLib UtilLib)
target_include_directories(ParserLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
