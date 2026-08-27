#ifndef GRAPHBLAS_BFS_H
#define GRAPHBLAS_BFS_H

#include "utils.h"

int graphblas_init(void);
void graphblas_finalize(void);
int graphblas_build_matrix(CSRMatrix* csr, void** A_out);
void graphblas_free_matrix(void* A_handle);
int graphblas_level_bfs(CSRMatrix* csr, void* A_handle, int start_vertex, int* level);
int graphblas_multisource_level_bfs(CSRMatrix* csr, void* A_handle, int* sources, int num_sources, int* level);

#endif 
