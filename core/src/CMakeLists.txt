file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/**.c)

add_library(DatabaseLib STATIC ${SOURCES})

target_include_directories(DatabaseLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(DatabaseLib UtilLib)
