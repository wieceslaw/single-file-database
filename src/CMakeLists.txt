#project source files
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/**.c)

#main project lib
add_library(DatabaseLibrary ${SOURCES})

target_include_directories(DatabaseLibrary PUBLIC "${PROJECT_SOURCE_DIR}/src")
