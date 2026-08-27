#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "test_utils.h"
#include "../main/graphblas_bfs.h"

void test_graphblas_level_bfs_correctness() {
    printf("\n Test: GraphBLAS Level BFS Correctness \n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    int n = csr->n;
    int* level = (int*)malloc(n * sizeof(int));
    assert(level != NULL);

    Graph* temp_g = create_test_graph();
    int start_vertex = find_max_degree_vertex(temp_g);
    delete_graph(temp_g);

    printf("  Стартовая вершина (макс. степень): %d\n", start_vertex);

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
    printf("Все вершины посещены\n");

    assert(level[start_vertex] == 0);
    printf("Расстояние до старта = 0\n");

    int expected_levels_0[5] = {0, 1, 2, 1, 2};
    int expected_levels_1[5] = {1, 0, 1, 2, 1};

    if (start_vertex == 0) {
        for (int i = 0; i < n; i++) {
            assert(level[i] == expected_levels_0[i]);
        }
        printf("Расстояния корректны для start=0\n");
    } else if (start_vertex == 1) {
        for (int i = 0; i < n; i++) {
            assert(level[i] == expected_levels_1[i]);
        }
        printf("Расстояния корректны для start=1\n");
    }

    for (int i = 0; i < n; i++) {
        if (level[i] != -1) {
            assert(level[i] < n);
        }
    }
    printf("Все расстояния корректны\n");

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
    printf("Каждая вершина имеет соседа на уровень выше\n");

    graphblas_free_matrix(A);
    free(level);
    delete_csr(csr);
    printf("Тест GraphBLAS Level BFS пройден!\n");
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

    printf("  Источники: ");
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
    printf("Все вершины посещены\n");

    int all_sources_zero = 1;
    for (int i = 0; i < num_sources; i++) {
        if (level[sources[i]] != 0) {
            all_sources_zero = 0;
            break;
        }
    }
    assert(all_sources_zero == 1);
    printf("Все источники имеют distance=0\n");

    assert(level[0] == 0);
    assert(level[1] == 0);
    assert(level[2] == 0);
    assert(level[3] == 0);
    assert(level[4] == 1);
    printf("Расстояния корректны для данного графа\n");

    for (int i = 0; i < n; i++) {
        if (level[i] != -1) {
            assert(level[i] < n);
        }
    }
    printf("Все расстояния корректны\n");

    int* level_single = (int*)malloc(n * sizeof(int));
    assert(level_single != NULL);
    graphblas_level_bfs(csr, A, 0, level_single);

    for (int i = 0; i < n; i++) {
        assert(level[i] <= level_single[i]);
    }
    printf("Multisource расстояния <= Single-source\n");

    int found_smaller = 0;
    for (int i = 0; i < n; i++) {
        if (level[i] < level_single[i]) {
            found_smaller = 1;
            break;
        }
    }
    assert(found_smaller == 1);
    printf("Есть вершины с меньшим расстоянием в multisource\n");

    graphblas_free_matrix(A);
    free(level);
    free(level_single);
    free(sources);
    delete_csr(csr);
    printf("Тест GraphBLAS Multisource Level BFS пройден!\n");
}

int main() {
    if (graphblas_init() != 0) {
        return 1;
    }

    test_graphblas_level_bfs_correctness();
    test_graphblas_multisource_level_bfs_correctness();

    graphblas_finalize();
    printf("Все тесты GraphBLAS BFS пройдены!\n");
    return 0;
}

