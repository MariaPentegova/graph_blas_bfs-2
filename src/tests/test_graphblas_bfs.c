#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "graphblas_bfs.h"

void test_graphblas_line_graph() {
    printf("\n[TEST 1] GraphBLAS BFS on line graph (0-1-2-3)\n");
    
    // Graph: 0--1--2--3
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
    
    // Check parent relationships
    assert(parent[0] == 0);
    assert(parent[1] == 0);
    assert(parent[2] == 1);
    assert(parent[3] == 2);
    
    // Check levels
    assert(level[0] == 0);
    assert(level[1] == 1);
    assert(level[2] == 2);
    assert(level[3] == 3);
    
    printf("  Time: %.6f ms\n", time_ms);
    
    free(parent);
    free(level);
    graphblas_free_graph(graph);
    graphblas_finalize();
    
    printf(" Test passed\n");
}

void test_graphblas_single_node() {
    printf("\n[TEST 2] GraphBLAS BFS on single node\n");
    
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
    assert(level[0] == 0);
    
    free(parent);
    free(level);
    graphblas_free_graph(graph);
    graphblas_finalize();
    
    printf(" Test passed\n");
}

int main() {
    printf("   GraphBLAS BFS Unit Tests            \n");
    
    test_graphblas_line_graph();
    test_graphblas_single_node();
  
    printf("   All GraphBLAS BFS tests PASSED!     \n");
    
    return 0;
}
