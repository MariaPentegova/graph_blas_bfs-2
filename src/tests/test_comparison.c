#include <assert.h>
#include <stdio.h>
#include "test_common.h"
#include "utils.h"
#include "classic_bfs.h"
#include "graphblas_bfs.h"

static void test_classic_equals_graphblas(TestGraph which) {
    CSRMatrix* csr = create_test_csr(which);
    void* A = NULL;
    int classic_parent[MAX_TEST_VERTICES];
    int graphblas_parent[MAX_TEST_VERTICES];
    int classic_depth[MAX_TEST_VERTICES];
    int graphblas_depth[MAX_TEST_VERTICES];
    int status = graphblas_build_matrix(csr, &A);
    assert(status == 0);

    for (int a = 0; a < csr->n; a++) {
        for (int b = a + 1; b < csr->n; b++) {
            const int sources[2] = {a, b};
            void* result = NULL;
            int classic_status = csr_multisource_bfs(csr, sources, 2, classic_parent);
            int graphblas_status = graphblas_multisource_parent_bfs(csr, A, sources, 2, &result);
            assert(classic_status == 0);
            assert(graphblas_status == 0);
            graphblas_result_to_array(result, csr->n, graphblas_parent);

            assert(bfs_tree_depths(csr, classic_parent, classic_depth) == 0);
            assert(bfs_tree_depths(csr, graphblas_parent, graphblas_depth) == 0);
            assert(arrays_equal(classic_depth, graphblas_depth, csr->n));
        }
    }

    graphblas_free_matrix(A);
    delete_csr(csr);
    printf("PASSED: Classic and GraphBLAS give equal results (%s graph)\n",
           which == UNDIRECTED_GRAPH ? "undirected" : "directed");
}

int main(void) {
    if (graphblas_init() != 0) {
        return 1;
    }

    test_classic_equals_graphblas(UNDIRECTED_GRAPH);
    test_classic_equals_graphblas(DIRECTED_GRAPH);

    graphblas_finalize();
    printf("All comparison tests passed\n");
    return 0;
}
