file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_LIST_DIR}/**.c)
foreach (TEST_SOURCE ${TEST_SOURCES})
    string(REGEX MATCH "^.*_(.*).c$" _ ${TEST_SOURCE})
    string(TOLOWER ${CMAKE_MATCH_1} TEST_SUFFIX)
    set(TEST_NAME "test_${TEST_SUFFIX}")
    add_executable(
            ${TEST_NAME}
            ${TEST_SOURCE}
    )
    message("TEST_NAME=${TEST_NAME}, TEST_SOURCE=${TEST_SOURCE}")
    target_include_directories(${TEST_NAME} INTERFACE ../../test)
    target_link_libraries(${TEST_NAME} CoreLib)

endforeach ()
