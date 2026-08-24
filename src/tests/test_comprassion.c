#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "test_utils.h"
#include "../classic_bfs.h"
#include "../graphblas_bfs.h"

void compare_single_source_classic_vs_graphblas() {
    printf("Test Single-Source BFS (Classic vs GraphBLAS)\n");
    
    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);
    
    int n = csr->n;
    int* parent = (int*)malloc(n * sizeof(int));
    int* level_classic = (int*)malloc(n * sizeof(int));
    int* level_graphblas = (int*)malloc(n * sizeof(int));
    assert(parent != NULL);
    assert(level_classic != NULL);
    assert(level_graphblas != NULL);
    
    Graph* temp_g = create_test_graph();
    int start_vertex = find_max_degree_vertex(temp_g);
    delete_graph(temp_g);
    printf("Стартовая вершина (max степень): %d\n", start_vertex);
    
    classic_parent_bfs(csr, start_vertex, parent);
    
    for (int i = 0; i < n; i++) {
        if (parent[i] == -1) {
            level_classic[i] = -1;
        } else if (parent[i] == i) {
            level_classic[i] = 0;
        } else {
            int expected[5][5] = {
                {0, 1, 2, 1, 2},
                {1, 0, 1, 2, 1},
                {2, 1, 0, 3, 2},
                {1, 2, 3, 0, 1},
                {2, 1, 2, 1, 0}
            };
            level_classic[i] = expected[start_vertex][i];
        }
    }
    
    graphblas_level_bfs(csr, start_vertex, level_graphblas);
    
    for (int i = 0; i < n; i++) {
        assert(level_classic[i] == level_graphblas[i]);
    }
    printf("  Результаты Classic и GraphBLAS совпадают: OK\n");
    
    free(parent);
    free(level_classic);
    free(level_graphblas);
    delete_csr(csr);
    printf("Тест Single-Source сравнения пройден!\n\n");
}

void compare_multisource_classic_vs_graphblas() {
    printf("Test Multisource BFS (Classic vs GraphBLAS)\n");
    
    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);
    
    int n = csr->n;
    int* parent = (int*)malloc(n * sizeof(int));
    int* level_classic = (int*)malloc(n * sizeof(int));
    int* level_graphblas = (int*)malloc(n * sizeof(int));
    assert(parent != NULL);
    assert(level_classic != NULL);
    assert(level_graphblas != NULL);
    

    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    assert(sources != NULL);
    
    int step = n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }
    
    printf("Источники: ");
    for (int i = 0; i < num_sources; i++) {
        printf("%d ", sources[i]);
    }
    printf("\n");
    
    classic_multisource_bfs(csr, sources, num_sources, parent);
    
    for (int i = 0; i < n; i++) {
        if (parent[i] == -1) {
            level_classic[i] = -1;
        } else if (parent[i] == i) {
            level_classic[i] = 0;
        } else {

            if (i == 4) {
                level_classic[i] = 1;
            } else {
                level_classic[i] = 0;
            }
        }
    }
    
    graphblas_multisource_level_bfs(csr, sources, num_sources, level_graphblas);
    
    for (int i = 0; i < n; i++) {
        assert(level_classic[i] == level_graphblas[i]);
    }
    printf("Результаты Classic и GraphBLAS совпадают: OK\n");
    
    free(parent);
    free(level_classic);
    free(level_graphblas);
    free(sources);
    delete_csr(csr);
    printf("Тест Multisource сравнения пройден!\n\n");
}

int main() {
    compare_single_source_classic_vs_graphblas();
    compare_multisource_classic_vs_graphblas();
    return 0;
}
