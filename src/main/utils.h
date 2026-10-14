#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

struct CSRMatrix {
    int n;
    int* row_ptr;
    int* col_idx;
};

typedef struct CSRMatrix CSRMatrix;

CSRMatrix* csr_from_edges(int n, const int* src, const int* dst, int num_edges, int symmetric);
CSRMatrix* load_csr(const char* filename);
void delete_csr(CSRMatrix* csr);

int find_max_degree_vertex(const CSRMatrix* csr);
int bfs_tree_depths(const CSRMatrix* csr, const int* parent, int* depth);

#ifdef __cplusplus
}
#endif

#endif
