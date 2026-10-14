#ifndef CLASSIC_BFS_H
#define CLASSIC_BFS_H

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

int csr_parent_bfs(const CSRMatrix* csr, int start_vertex, int* parent);

int csr_multisource_bfs(const CSRMatrix* csr, const int* sources, int num_sources, int* parent);

#ifdef __cplusplus
}
#endif

#endif
