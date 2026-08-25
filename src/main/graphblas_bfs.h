#ifndef GRAPHBLAS_BFS_H
#define GRAPHBLAS_BFS_H

#include <stdio.h>
#include <stdlib.h>
#include <GraphBLAS.h>   
#include "utils.h"

csr_to_graphblas(CSRMatrix* csr, GrB_Matrix* A);
int graphblas_level_bfs(CSRMatrix* csr, int start_vertex, int* level);
int graphblas_multisource_level_bfs(CSRMatrix* csr, int* sources, int num_sources, int* level);

#endif
