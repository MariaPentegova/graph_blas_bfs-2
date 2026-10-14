#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "test_common.h"
#include "graphblas_bfs.h"

CSRMatrix* create_test_csr(TestGraph which) {
    static const int undirected_src[5] = {0, 1, 0, 1, 3};
    static const int undirected_dst[5] = {1, 2, 3, 4, 4};
    static const int directed_src[3] = {0, 1, 3};
    static const int directed_dst[3] = {1, 2, 0};

    CSRMatrix* csr = NULL;
    if (which == UNDIRECTED_GRAPH) {
        csr = csr_from_edges(5, undirected_src, undirected_dst, 5, 1);
    } else {
        csr = csr_from_edges(4, directed_src, directed_dst, 3, 0);
    }
    assert(csr != NULL);
    return csr;
}

const int* expected_distances(TestGraph which, int start) {
    static const int undirected[5][5] = {
        {0, 1, 2, 1, 2},
        {1, 0, 1, 2, 1},
        {2, 1, 0, 3, 2},
        {1, 2, 3, 0, 1},
        {2, 1, 2, 1, 0}
    };
    static const int directed[4][4] = {
        { 0,  1,  2, -1},
        {-1,  0,  1, -1},
        {-1, -1,  0, -1},
        { 1,  2,  3,  0}
    };
    return which == UNDIRECTED_GRAPH ? undirected[start] : directed[start];
}

bool arrays_equal(const int* a, const int* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

void graphblas_result_to_array(void* result, int n, int* parent) {
    assert(result != NULL);
    int status = graphblas_parent_to_array(result, n, parent);
    assert(status == 0);
    graphblas_free_vector(result);
}
