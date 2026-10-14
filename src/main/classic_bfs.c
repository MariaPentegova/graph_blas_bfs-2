#include <stdio.h>
#include <stdlib.h>
#include "classic_bfs.h"

static int run_bfs(const CSRMatrix* csr, const int* sources, int num_sources, int* parent) {
    int n = csr->n;
    for (int i = 0; i < n; i++) {
        parent[i] = -1;
    }

    int* queue = (int*)malloc((size_t)n * sizeof(int));
    if (queue == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for the queue\n");
        return -1;
    }

    int head = 0;
    int tail = 0;
    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        if (s >= 0 && s < n && parent[s] == -1) {
            parent[s] = s;
            queue[tail++] = s;
        }
    }

    while (head < tail) {
        int v = queue[head++];
        for (int i = csr->row_ptr[v]; i < csr->row_ptr[v + 1]; i++) {
            int neighbor = csr->col_idx[i];
            if (parent[neighbor] == -1) {
                parent[neighbor] = v;
                queue[tail++] = neighbor;
            }
        }
    }

    free(queue);
    return 0;
}

int csr_parent_bfs(const CSRMatrix* csr, int start_vertex, int* parent) {
    if (csr == NULL || parent == NULL || start_vertex < 0 || start_vertex >= csr->n) {
        return -1;
    }
    return run_bfs(csr, &start_vertex, 1, parent);
}

int csr_multisource_bfs(const CSRMatrix* csr, const int* sources, int num_sources, int* parent) {
    if (csr == NULL || parent == NULL || sources == NULL || num_sources <= 0) {
        return -1;
    }
    return run_bfs(csr, sources, num_sources, parent);
}
