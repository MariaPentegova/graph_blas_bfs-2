#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "graphblas_bfs.h"

void test_graphblas_line_graph() {
    printf("Test 1: GraphBLAS BFS on line graph (0-1-2-3)\n");
    
    int row_ptr[] = {0, 1, 3, 5, 6};
    int col_idx[] = {1, 0, 2, 1, 3, 2};
    double values[] = {1, 1, 1, 1, 1, 1};
    int n = 4;
    int nnz = 6;
    
    graphblas_init();
    GraphBLASGraph* graph = graphblas_create_graph(n, row_ptr, col_idx, values, nnz);
    
    int* level = NULL;
    double time_ms;
    int* parent = graphblas_bfs(graph, 0, &time_ms, &level);
    
    assert(parent[0] == 0);
    assert(level[0] == 0);
    assert(level[1] == 1);
    
    printf("  Time: %.6f ms\n", time_ms);
    
    free(parent);
    if (level) free(level);
    graphblas_free_graph(graph);
    graphblas_finalize();
    
    printf("  OK\n");
}

void test_graphblas_single_node() {
    printf("Test 2: GraphBLAS BFS on single node\n");
    
    int row_ptr[] = {0, 0};
    int col_idx[] = {};
    double values[] = {};
    int n = 1;
    int nnz = 0;
    
    graphblas_init();
    GraphBLASGraph* graph = graphblas_create_graph(n, row_ptr, col_idx, values, nnz);
    
    int* level = NULL;
    double time_ms;
    int* parent = graphblas_bfs(graph, 0, &time_ms, &level);
    
    assert(parent[0] == 0);
    if (level) {
        assert(level[0] == 0);
    }
    
    free(parent);
    if (level) free(level);
    graphblas_free_graph(graph);
    graphblas_finalize();
    
    printf("  OK\n");
}

int main() {
    test_graphblas_line_graph();
    test_graphblas_single_node();
    
    printf("All GraphBLAS BFS tests passed\n");
    return 0;
}