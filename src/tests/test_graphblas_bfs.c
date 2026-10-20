#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "test_utils.h"
#include "../main/graphblas_bfs.h"

void test_graphblas_level_bfs_correctness() {
    printf("\n Test GraphBLAS Level BFS Correctness \n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    int n = csr->n;
    int* level = (int*)malloc(n * sizeof(int));
    assert(level != NULL);

    Graph* temp_g = create_test_graph();
    int start_vertex = find_max_degree_vertex(temp_g);
    delete_graph(temp_g);

    printf(" Start vertex (max degree): %d\n", start_vertex);

    void* A = NULL;
    assert(graphblas_build_matrix(csr, &A) == 0);

    int result = graphblas_level_bfs(csr, A, start_vertex, level);
    assert(result == 0);

    int all_visited = 1;
    for (int i = 0; i < n; i++) {
        if (level[i] == -1) {
            all_visited = 0;
            break;
        }
    }
    assert(all_visited == 1);
    printf("All vertices are visited\n");

    assert(level[start_vertex] == 0);
    printf("Distance to the start = 0\n");

    int expected_levels_0[5] = {0, 1, 2, 1, 2};
    int expected_levels_1[5] = {1, 0, 1, 2, 1};

    if (start_vertex == 0) {
        for (int i = 0; i < n; i++) {
            assert(level[i] == expected_levels_0[i]);
        }
        printf("Distances are valid for start=0\n");
    } else if (start_vertex == 1) {
        for (int i = 0; i < n; i++) {
            assert(level[i] == expected_levels_1[i]);
        }
        printf("Distances are valid for start=1\n");
    }

    for (int i = 0; i < n; i++) {
        if (level[i] != -1) {
            assert(level[i] < n);
        }
    }
    printf("All distances are valid\n");

    int all_have_parent = 1;
    for (int i = 0; i < n; i++) {
        if (level[i] > 0) {
            int found = 0;
            int start = csr->row_ptr[i];
            int end = csr->row_ptr[i + 1];
            for (int j = start; j < end; j++) {
                int neighbor = csr->col_idx[j];
                if (level[neighbor] == level[i] - 1) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                all_have_parent = 0;
                break;
            }
        }
    }
    assert(all_have_parent == 1);
    printf("Each vertex has a neighbor one level higher.\n");

    graphblas_free_matrix(A);
    free(level);
    delete_csr(csr);
    printf("Test GraphBLAS Level BFS passed!\n");
}

void test_graphblas_multisource_level_bfs_correctness() {
    printf("\nTest GraphBLAS Multisource Level BFS Correctness \n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    int n = csr->n;
    int* level = (int*)malloc(n * sizeof(int));
    assert(level != NULL);

    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    assert(sources != NULL);

    int step = n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }

    printf(" Sources: ");
    for (int i = 0; i < num_sources; i++) {
        printf("%d ", sources[i]);
    }
    printf("\n");

    void* A = NULL;
    assert(graphblas_build_matrix(csr, &A) == 0);

    int result = graphblas_multisource_level_bfs(csr, A, sources, num_sources, level);
    assert(result == 0);

    int all_visited = 1;
    for (int i = 0; i < n; i++) {
        if (level[i] == -1) {
            all_visited = 0;
            break;
        }
    }
    assert(all_visited == 1);
    printf("All vertices are visited\n");

    int all_sources_zero = 1;
    for (int i = 0; i < num_sources; i++) {
        if (level[sources[i]] != 0) {
            all_sources_zero = 0;
            break;
        }
    }
    assert(all_sources_zero == 1);
    printf("All sources have distance=0\n");

    assert(level[0] == 0);
    assert(level[1] == 0);
    assert(level[2] == 0);
    assert(level[3] == 0);
    assert(level[4] == 1);
    printf("The distances are correct for this graph.\n");

    for (int i = 0; i < n; i++) {
        if (level[i] != -1) {
            assert(level[i] < n);
        }
    }
    printf("All distances are valid\n");

    int* level_single = (int*)malloc(n * sizeof(int));
    assert(level_single != NULL);
    graphblas_level_bfs(csr, A, 0, level_single);

    for (int i = 0; i < n; i++) {
        assert(level[i] <= level_single[i]);
    }
    printf("Multisource distances <= Single-source\n");

    int found_smaller = 0;
    for (int i = 0; i < n; i++) {
        if (level[i] < level_single[i]) {
            found_smaller = 1;
            break;
        }
    }
    assert(found_smaller == 1);
    printf("There are vertices with a smaller distance in the multi-source case.\n");

    graphblas_free_matrix(A);
    free(level);
    free(level_single);
    free(sources);
    delete_csr(csr);
    printf("Test GraphBLAS Multisource Level BFS passed!\n");
}

int main() {
    if (graphblas_init() != 0) {
        return 1;
    }

    test_graphblas_level_bfs_correctness();
    test_graphblas_multisource_level_bfs_correctness();

    graphblas_finalize();
    printf("All Tests GraphBLAS BFS passed!\n");
    return 0;
}

