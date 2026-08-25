cmake_minimum_required(VERSION 3.10)
project(GraphBFS VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================
# Поиск GraphBLAS (без find_package)
# ============================================
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

# ============================================
# Пути к файлам
# ============================================
set(MAIN_DIR ${PROJECT_SOURCE_DIR}/src/main)
set(TESTS_DIR ${PROJECT_SOURCE_DIR}/src/tests)

# ============================================
# Исходные файлы
# ============================================
set(UTILS_SOURCES
    ${MAIN_DIR}/utils.c
)

set(CLASSIC_BFS_SOURCES
    ${MAIN_DIR}/classic_bfs.c
)

set(GRAPHBLAS_BFS_SOURCES
    ${MAIN_DIR}/graphblas_bfs.c
)

set(TEST_UTILS_SOURCES
    ${TESTS_DIR}/test_utils.c
)

# ============================================
# Основной исполняемый файл
# ============================================
add_executable(bfs_analysis
    ${MAIN_DIR}/main.c
    ${UTILS_SOURCES}
    ${CLASSIC_BFS_SOURCES}
    ${GRAPHBLAS_BFS_SOURCES}
)

target_include_directories(bfs_analysis PRIVATE
    ${MAIN_DIR}
    ${GRAPHBLAS_INCLUDE_DIR}
)

target_link_libraries(bfs_analysis
    ${GRAPHBLAS_LIBRARY}
    m
    pthread
)

# ============================================
# Бенчмарк
# ============================================
add_executable(bfs_benchmark
    ${MAIN_DIR}/benchmark.c
    ${UTILS_SOURCES}
    ${CLASSIC_BFS_SOURCES}
    ${GRAPHBLAS_BFS_SOURCES}
)

target_include_directories(bfs_benchmark PRIVATE
    ${MAIN_DIR}
    ${GRAPHBLAS_INCLUDE_DIR}
)

target_link_libraries(bfs_benchmark
    ${GRAPHBLAS_LIBRARY}
    m
    pthread
)

# ============================================
# Тесты
# ============================================
enable_testing()

add_executable(test_classic_bfs
    ${TESTS_DIR}/test_classic_bfs.c
    ${TEST_UTILS_SOURCES}
    ${UTILS_SOURCES}
    ${CLASSIC_BFS_SOURCES}
)

target_include_directories(test_classic_bfs PRIVATE
    ${MAIN_DIR}
    ${TESTS_DIR}
    ${GRAPHBLAS_INCLUDE_DIR}
)

target_link_libraries(test_classic_bfs
    m
    pthread
)

add_executable(test_graphblas_bfs
    ${TESTS_DIR}/test_graphblas_bfs.c
    ${TEST_UTILS_SOURCES}
    ${UTILS_SOURCES}
    ${GRAPHBLAS_BFS_SOURCES}
)

target_include_directories(test_graphblas_bfs PRIVATE
    ${MAIN_DIR}
    ${TESTS_DIR}
    ${GRAPHBLAS_INCLUDE_DIR}
)

target_link_libraries(test_graphblas_bfs
    ${GRAPHBLAS_LIBRARY}
    m
    pthread
)

add_executable(test_comparison
    ${TESTS_DIR}/test_comparison.c
    ${TEST_UTILS_SOURCES}
    ${UTILS_SOURCES}
    ${CLASSIC_BFS_SOURCES}
    ${GRAPHBLAS_BFS_SOURCES}
)

target_include_directories(test_comparison PRIVATE
    ${MAIN_DIR}
    ${TESTS_DIR}
    ${GRAPHBLAS_INCLUDE_DIR}
)

target_link_libraries(test_comparison
    ${GRAPHBLAS_LIBRARY}
    m
    pthread
)

# ============================================
# Добавление тестов в CTest
# ============================================
add_test(NAME ClassicBFS COMMAND test_classic_bfs)
add_test(NAME GraphBLASBFS COMMAND test_graphblas_bfs)
add_test(NAME Comparison COMMAND test_comparison)

message(STATUS "Build configuration ready")
