#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "test_common.h"
#include "utils.h"
#include "classic_bfs.h"
#include "graphblas_bfs.h"

static bool is_expected_result(const CSRMatrix* csr, const int* parent, const int* expected) {
    int depth[MAX_TEST_VERTICES];
    assert(csr->n <= MAX_TEST_VERTICES);
    return bfs_tree_depths(csr, parent, depth) == 0 && arrays_equal(depth, expected, csr->n);
}

static void check_both_implementations(TestGraph which, const int* sources, int num_sources,
                                       const int* expected) {
    CSRMatrix* csr = create_test_csr(which);
    void* A = NULL;
    void* result = NULL;
    int parent[MAX_TEST_VERTICES];
    int status = graphblas_build_matrix(csr, &A);
    assert(status == 0);

    status = csr_multisource_bfs(csr, sources, num_sources, parent);
    assert(status == 0);
    assert(is_expected_result(csr, parent, expected));

    status = graphblas_multisource_parent_bfs(csr, A, sources, num_sources, &result);
    assert(status == 0);
    graphblas_result_to_array(result, csr->n, parent);
    assert(is_expected_result(csr, parent, expected));

    if (num_sources == 1) {
        status = csr_parent_bfs(csr, sources[0], parent);
        assert(status == 0);
        assert(is_expected_result(csr, parent, expected));

        status = graphblas_parent_bfs(csr, A, sources[0], &result);
        assert(status == 0);
        graphblas_result_to_array(result, csr->n, parent);
        assert(is_expected_result(csr, parent, expected));
    }

    graphblas_free_matrix(A);
    delete_csr(csr);
}

static void test_single_source(void) {
    for (int start = 0; start < 5; start++) {
        check_both_implementations(UNDIRECTED_GRAPH, &start, 1,
                                   expected_distances(UNDIRECTED_GRAPH, start));
    }
    for (int start = 0; start < 4; start++) {
        check_both_implementations(DIRECTED_GRAPH, &start, 1,
                                   expected_distances(DIRECTED_GRAPH, start));
    }
    printf("PASSED: single-source BFS from every vertex\n");
}

static void test_multisource(void) {
    const int sources[4] = {0, 1, 2, 3};
    const int expected[5] = {0, 0, 0, 0, 1};
    check_both_implementations(UNDIRECTED_GRAPH, sources, 4, expected);

    const int directed_sources[2] = {1, 3};
    const int directed_expected[4] = {1, 0, 1, 0};
    check_both_implementations(DIRECTED_GRAPH, directed_sources, 2, directed_expected);

    const int repeated[3] = {1, 1, 1};
    check_both_implementations(UNDIRECTED_GRAPH, repeated, 3,
                               expected_distances(UNDIRECTED_GRAPH, 1));

    const int partly_invalid[3] = {-5, 1, 100};
    check_both_implementations(UNDIRECTED_GRAPH, partly_invalid, 3,
                               expected_distances(UNDIRECTED_GRAPH, 1));

    const int all_invalid[2] = {-1, 5};
    const int nothing_reached[5] = {-1, -1, -1, -1, -1};
    check_both_implementations(UNDIRECTED_GRAPH, all_invalid, 2, nothing_reached);

    printf("PASSED: multisource BFS\n");
}

static void test_directed_exact_parents(void) {
    CSRMatrix* csr = create_test_csr(DIRECTED_GRAPH);
    void* A = NULL;
    void* result = NULL;
    int parent[4];
    const int expected_parent[4] = {0, 0, 1, -1};
    int status = graphblas_build_matrix(csr, &A);
    assert(status == 0);

    status = csr_parent_bfs(csr, 0, parent);
    assert(status == 0);
    assert(arrays_equal(parent, expected_parent, 4));

    status = graphblas_parent_bfs(csr, A, 0, &result);
    assert(status == 0);
    graphblas_result_to_array(result, csr->n, parent);
    assert(arrays_equal(parent, expected_parent, 4));

    graphblas_free_matrix(A);
    delete_csr(csr);
    printf("PASSED: edge directions are respected\n");
}

static void test_invalid_arguments(void) {
    CSRMatrix* csr = create_test_csr(UNDIRECTED_GRAPH);
    void* A = NULL;
    void* result = NULL;
    int parent[5];
    const int sources[1] = {0};
    int status = graphblas_build_matrix(csr, &A);
    assert(status == 0);

    int results[] = {
        csr_parent_bfs(csr, -1, parent),
        csr_parent_bfs(csr, csr->n, parent),
        csr_parent_bfs(csr, 0, NULL),
        csr_parent_bfs(NULL, 0, parent),
        csr_multisource_bfs(csr, NULL, 1, parent),
        csr_multisource_bfs(csr, sources, 0, parent),
        graphblas_parent_bfs(csr, A, -1, &result),
        graphblas_parent_bfs(csr, A, csr->n, &result),
        graphblas_parent_bfs(csr, A, 0, NULL),
        graphblas_parent_bfs(csr, NULL, 0, &result),
        graphblas_multisource_parent_bfs(csr, A, NULL, 1, &result),
        graphblas_multisource_parent_bfs(csr, A, sources, 0, &result),
        graphblas_parent_to_array(NULL, csr->n, parent)
    };
    for (size_t i = 0; i < sizeof(results) / sizeof(results[0]); i++) {
        assert(results[i] == -1);
    }

    result = A;
    status = graphblas_parent_bfs(csr, A, -1, &result);
    assert(status == -1);
    assert(result == NULL);

    result = A;
    status = graphblas_multisource_parent_bfs(csr, A, NULL, 1, &result);
    assert(status == -1);
    assert(result == NULL);

    graphblas_free_matrix(A);
    delete_csr(csr);
    printf("PASSED: invalid arguments are rejected\n");
}

static void test_isolated_and_loop(void) {
    static const int src[3] = {0, 0, 1};
    static const int dst[3] = {0, 1, 2};
    CSRMatrix* csr = csr_from_edges(4, src, dst, 3, 0);
    assert(csr != NULL);

    void* A = NULL;
    assert(graphblas_build_matrix(csr, &A) == 0);

    const int expected_from_0[4] = {0, 1, 2, -1};
    const int expected_from_3[4] = {-1, -1, -1, 0};

    for (int which = 0; which < 2; which++) {
        const int start = (which == 0) ? 0 : 3;
        const int* expected = (which == 0) ? expected_from_0 : expected_from_3;
        int parent[4];
        int depth[4];
        void* result = NULL;

        assert(csr_parent_bfs(csr, start, parent) == 0);
        assert(bfs_tree_depths(csr, parent, depth) == 0);
        assert(arrays_equal(depth, expected, 4));

        assert(graphblas_parent_bfs(csr, A, start, &result) == 0);
        graphblas_result_to_array(result, csr->n, parent);
        assert(bfs_tree_depths(csr, parent, depth) == 0);
        assert(arrays_equal(depth, expected, 4));
    }

    graphblas_free_matrix(A);
    delete_csr(csr);
    printf("PASSED: isolated vertex and loop\n");
}

static void test_graphblas_all_invalid_sources(void) {
    CSRMatrix* csr = create_test_csr(UNDIRECTED_GRAPH);
    void* A = NULL;
    void* result = NULL;
    int parent[MAX_TEST_VERTICES];
    const int all_invalid[2] = {-1, 5};
    assert(graphblas_build_matrix(csr, &A) == 0);

    int status = graphblas_multisource_parent_bfs(csr, A, all_invalid, 2, &result);
    assert(status == 0);
    assert(result != NULL);
    assert(graphblas_parent_to_array(result, csr->n, parent) == 0);
    for (int i = 0; i < csr->n; i++) {
        assert(parent[i] == -1);
    }

    graphblas_free_vector(result);
    graphblas_free_matrix(A);
    delete_csr(csr);
    printf("PASSED: all-invalid sources give an empty result vector\n");
}

int main(void) {
    if (graphblas_init() != 0) {
        return 1;
    }

    test_single_source();
    test_multisource();
    test_directed_exact_parents();
    test_invalid_arguments();
    test_isolated_and_loop();
    test_graphblas_all_invalid_sources();

    graphblas_finalize();
    printf("All BFS tests passed\n");
    return 0;
}
