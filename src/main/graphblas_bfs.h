#ifndef GRAPHBLAS_BFS_H
#define GRAPHBLAS_BFS_H

#include <GraphBLAS.h>

typedef struct {
    GrB_Matrix adjacency;  // adjacency matrix (boolean)
    int n;                  // number of nodes
    int* row_ptr;           // CSR row pointers (сохраняем для надёжности)
    int* col_idx;           // CSR column indices
    int nnz;                // number of edges
} GraphBLASGraph;

// GraphBLAS initialization and finalization
void graphblas_init();
void graphblas_finalize();

// Create graph from CSR format
GraphBLASGraph* graphblas_create_graph(int n, int* row_ptr, int* col_idx, double* values, int nnz);
void graphblas_free_graph(GraphBLASGraph* graph);

// Single source BFS 
int* graphblas_bfs(GraphBLASGraph* graph, int source, double* time_ms, int** level);

// Multi-source BFS
int* graphblas_bfs_multisource(GraphBLASGraph* graph, int* sources, int num_sources, 
                                double* time_ms, int** level);

#endif