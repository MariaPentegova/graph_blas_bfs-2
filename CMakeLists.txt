cmake_minimum_required(VERSION 3.10)
project(GraphBFS VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
if(NOT IS_MULTI_CONFIG)
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()
find_package(Boost REQUIRED)
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

find_package(Threads REQUIRED)

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

set(MAIN_DIR ${PROJECT_SOURCE_DIR}/src/main)
set(TESTS_DIR ${PROJECT_SOURCE_DIR}/src/tests)

add_library(bfs_core STATIC
    ${MAIN_DIR}/utils.c
    ${MAIN_DIR}/classic_bfs.c
    ${MAIN_DIR}/graphblas_bfs.c
)
target_include_directories(bfs_core PUBLIC ${MAIN_DIR} ${GRAPHBLAS_INCLUDE_DIR})
target_link_libraries(bfs_core PUBLIC ${GRAPHBLAS_LIBRARY} Threads::Threads m)

add_executable(bfs_benchmark ${MAIN_DIR}/benchmark.cpp)
target_include_directories(bfs_benchmark PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(bfs_benchmark PRIVATE bfs_core)

enable_testing()

add_library(test_common STATIC ${TESTS_DIR}/test_common.c)
target_include_directories(test_common PUBLIC ${TESTS_DIR})
target_link_libraries(test_common PUBLIC bfs_core)
target_compile_options(test_common PUBLIC -UNDEBUG)

foreach(test_name test_load test_check test_bfs test_comparison)
    add_executable(${test_name} ${TESTS_DIR}/${test_name}.c)
    target_link_libraries(${test_name} PRIVATE test_common)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
