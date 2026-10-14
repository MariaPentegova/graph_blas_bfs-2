#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdbool.h>
#include "utils.h"

#define MAX_TEST_VERTICES 5

typedef enum {
    UNDIRECTED_GRAPH,
    DIRECTED_GRAPH
} TestGraph;

CSRMatrix* create_test_csr(TestGraph which);

const int* expected_distances(TestGraph which, int start);

bool arrays_equal(const int* a, const int* b, int n);

void graphblas_result_to_array(void* result, int n, int* parent);

#endif
