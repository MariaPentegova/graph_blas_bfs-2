cmake_minimum_required(VERSION 3.10)
project(GraphBFS VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Поиск GraphBLAS
find_path(GRAPHBLAS_INCLUDE_DIR
    NAMES GraphBLAS.h
    PATHS /usr/local/include/suitesparse /usr/include/suitesparse /usr/local/include /usr/include
    PATH_SUFFIXES suitesparse
)

find_library(GRAPHBLAS_LIBRARY
    NAMES graphblas libgraphblas
    PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu
)

if(NOT GRAPHBLAS_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find GraphBLAS.h")
endif()

if(NOT GRAPHBLAS_LIBRARY)
    message(FATAL_ERROR "Could not find graphblas library")
endif()

message(STATUS "GraphBLAS include: ${GRAPHBLAS_INCLUDE_DIR}")
message(STATUS "GraphBLAS library: ${GRAPHBLAS_LIBRARY}")

# Include directories
include_directories(${GRAPHBLAS_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src/main)

# Source files
set(CLASSIC_BFS_SOURCES
    src/main/classic_bfs.c
)

set(GRAPHBLAS_BFS_SOURCES
    src/main/graphblas_bfs.c
)

set(UTILS_SOURCES
    src/main/utils.c
)

# Main executable
add_executable(bfs_analysis
    src/main/main.c
    ${CLASSIC_BFS_SOURCES}
    ${GRAPHBLAS_BFS_SOURCES}
    ${UTILS_SOURCES}
)

target_link_libraries(bfs_analysis ${GRAPHBLAS_LIBRARY} m pthread)

# Tests - добавляем линковку с GraphBLAS
enable_testing()

add_executable(test_classic_bfs
    src/tests/test_classic_bfs.c
    ${CLASSIC_BFS_SOURCES}
    ${UTILS_SOURCES}
)

# Важно: добавляем линковку с GraphBLAS для тестов
target_link_libraries(test_classic_bfs ${GRAPHBLAS_LIBRARY} m pthread)

add_executable(test_graphblas_bfs
    src/tests/test_graphblas_bfs.c
    ${GRAPHBLAS_BFS_SOURCES}
    ${UTILS_SOURCES}
)

target_link_libraries(test_graphblas_bfs ${GRAPHBLAS_LIBRARY} m pthread)

# Добавляем include directory для тестов
target_include_directories(test_classic_bfs PRIVATE ${GRAPHBLAS_INCLUDE_DIR})
target_include_directories(test_graphblas_bfs PRIVATE ${GRAPHBLAS_INCLUDE_DIR})

add_test(NAME ClassicBFS COMMAND test_classic_bfs)
add_test(NAME GraphBLASBFS COMMAND test_graphblas_bfs)

message(STATUS "Build configuration ready")