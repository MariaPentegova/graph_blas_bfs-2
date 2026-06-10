cmake_minimum_required(VERSION 3.10)
project(GraphBFS VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find SuiteSparse GraphBLAS
find_package(SuiteSparse REQUIRED COMPONENTS GraphBLAS)

# Include directories
include_directories(${SuiteSparse_INCLUDE_DIRS})
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

target_link_libraries(bfs_analysis ${SuiteSparse_LIBRARIES} m)

# Tests
enable_testing()

add_executable(test_classic_bfs
    src/tests/test_classic_bfs.c
    ${CLASSIC_BFS_SOURCES}
    ${UTILS_SOURCES}
)

add_executable(test_graphblas_bfs
    src/tests/test_graphblas_bfs.c
    ${GRAPHBLAS_BFS_SOURCES}
    ${UTILS_SOURCES}
)

target_link_libraries(test_classic_bfs ${SuiteSparse_LIBRARIES} m)
target_link_libraries(test_graphblas_bfs ${SuiteSparse_LIBRARIES} m)

add_test(NAME ClassicBFS COMMAND test_classic_bfs)
add_test(NAME GraphBLASBFS COMMAND test_graphblas_bfs)

# Print status
message(STATUS "Build configuration:")
message(STATUS "  SuiteSparse include: ${SuiteSparse_INCLUDE_DIRS}")
message(STATUS "  SuiteSparse libs: ${SuiteSparse_LIBRARIES}")
