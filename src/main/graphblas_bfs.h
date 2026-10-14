#ifndef GRAPHBLAS_BFS_H
#define GRAPHBLAS_BFS_H

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

int graphblas_init(void);
void graphblas_finalize(void);
int graphblas_num_threads(void);

int graphblas_build_matrix(const CSRMatrix* csr, void** A_out);
void graphblas_free_matrix(void* A_handle);

int graphblas_parent_bfs(const CSRMatrix* csr, void* A_handle, int start_vertex,
                         void** parent_out);
int graphblas_multisource_parent_bfs(const CSRMatrix* csr, void* A_handle,
                                     const int* sources, int num_sources, void** parent_out);

int graphblas_parent_to_array(void* parent_handle, int n, int* parent);
void graphblas_free_vector(void* vector_handle);

#ifdef __cplusplus
}
#endif

#endif
