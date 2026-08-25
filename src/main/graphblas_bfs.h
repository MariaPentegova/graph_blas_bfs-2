#ifndef GRAPHBLAS_BFS_H
#define GRAPHBLAS_BFS_H

int csr_to_graphblas(CSRMatrix* csr, void** A);
int graphblas_level_bfs(CSRMatrix* csr, int start_vertex, int* level);
int graphblas_multisource_level_bfs(CSRMatrix* csr, int* sources, int num_sources, int* level);

#endif
