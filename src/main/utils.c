#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define MM_MAX_LINE_LENGTH 1026

#define DEPTH_UNKNOWN (-2)

void delete_csr(CSRMatrix* csr) {
    if (csr == NULL) {
        return;
    }
    free(csr->row_ptr);
    free(csr->col_idx);
    free(csr);
}

static int compare_ints(const void* a, const void* b) {
    int x = *(const int*)a;
    int y = *(const int*)b;
    return (x > y) - (x < y);
}

static void sort_rows_and_remove_duplicates(CSRMatrix* csr) {
    int write = 0;
    int begin = 0;
    for (int v = 0; v < csr->n; v++) {
        int end = csr->row_ptr[v + 1];
        qsort(csr->col_idx + begin, (size_t)(end - begin), sizeof(int), compare_ints);

        csr->row_ptr[v] = write;
        for (int i = begin; i < end; i++) {
            if (write == csr->row_ptr[v] || csr->col_idx[write - 1] != csr->col_idx[i]) {
                csr->col_idx[write++] = csr->col_idx[i];
            }
        }
        begin = end;
    }
    csr->row_ptr[csr->n] = write;
}

CSRMatrix* csr_from_edges(int n, const int* src, const int* dst, int num_edges, int symmetric) {
    if (n <= 0 || num_edges < 0 || (num_edges > 0 && (src == NULL || dst == NULL))) {
        return NULL;
    }
    if (symmetric && num_edges > INT_MAX / 2) {
        fprintf(stderr, "Error: too many edges (%d)\n", num_edges);
        return NULL;
    }
    for (int e = 0; e < num_edges; e++) {
        if (src[e] < 0 || src[e] >= n || dst[e] < 0 || dst[e] >= n) {
            fprintf(stderr, "Error: invalid vertex indices (%d, %d)\n", src[e], dst[e]);
            return NULL;
        }
    }

    CSRMatrix* csr = (CSRMatrix*)calloc(1, sizeof(CSRMatrix));
    int* next = (int*)malloc((size_t)n * sizeof(int));
    if (csr == NULL || next == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for CSR\n");
        free(csr);
        free(next);
        return NULL;
    }
    csr->n = n;
    csr->row_ptr = (int*)calloc((size_t)n + 1, sizeof(int));
    if (csr->row_ptr == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for CSR\n");
        free(next);
        delete_csr(csr);
        return NULL;
    }

    for (int e = 0; e < num_edges; e++) {
        csr->row_ptr[src[e] + 1]++;
        if (symmetric && src[e] != dst[e]) {
            csr->row_ptr[dst[e] + 1]++;
        }
    }
    for (int v = 0; v < n; v++) {
        csr->row_ptr[v + 1] += csr->row_ptr[v];
    }

    int total = csr->row_ptr[n];
    csr->col_idx = (int*)malloc((size_t)(total > 0 ? total : 1) * sizeof(int));
    if (csr->col_idx == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for CSR\n");
        free(next);
        delete_csr(csr);
        return NULL;
    }

    memcpy(next, csr->row_ptr, (size_t)n * sizeof(int));
    for (int e = 0; e < num_edges; e++) {
        csr->col_idx[next[src[e]]++] = dst[e];
        if (symmetric && src[e] != dst[e]) {
            csr->col_idx[next[dst[e]]++] = src[e];
        }
    }
    free(next);

    sort_rows_and_remove_duplicates(csr);
    return csr;
}

static int read_header(FILE* f, int* is_symmetric) {
    char line[MM_MAX_LINE_LENGTH];

    if (fgets(line, sizeof(line), f) == NULL) {
        fprintf(stderr, "Error: empty file\n");
        return -1;
    }
    if (strstr(line, "%%MatrixMarket") == NULL) {
        fprintf(stderr, "Error: not a Matrix Market file\n");
        return -1;
    }
    if (strstr(line, "coordinate") == NULL) {
        fprintf(stderr, "Error: only the coordinate format is supported\n");
        return -1;
    }
    if (strstr(line, "pattern") == NULL) {
        fprintf(stderr, "Error: only pattern matrices (without weights) are supported\n");
        return -1;
    }

    if (strstr(line, "symmetric") != NULL) {
        *is_symmetric = 1;
    } else if (strstr(line, "general") != NULL) {
        *is_symmetric = 0;
    } else {
        fprintf(stderr, "Error: only symmetric and general matrices are supported\n");
        return -1;
    }
    return 0;
}

static int read_sizes(FILE* f, int* rows, int* cols, int* entries) {
    char line[MM_MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), f) != NULL) {
        if (line[0] == '%') {
            continue;
        }
        if (sscanf(line, "%d %d %d", rows, cols, entries) != 3) {
            break;
        }
        return 0;
    }
    fprintf(stderr, "Error: failed to read matrix sizes\n");
    return -1;
}

static int read_edges(FILE* f, int entries, int* src, int* dst) {
    char line[MM_MAX_LINE_LENGTH];

    for (int e = 0; e < entries; e++) {
        if (fgets(line, sizeof(line), f) == NULL) {
            fprintf(stderr, "Error: failed to read edge %d\n", e);
            return -1;
        }
        char* end_first = NULL;
        char* end_second = NULL;
        long first = strtol(line, &end_first, 10);
        long second = strtol(end_first, &end_second, 10);
        if (end_first == line || end_second == end_first ||
            first < 1 || first > INT_MAX || second < 1 || second > INT_MAX) {
            fprintf(stderr, "Error: failed to read edge %d\n", e);
            return -1;
        }
        src[e] = (int)first - 1;
        dst[e] = (int)second - 1;
    }
    return 0;
}

CSRMatrix* load_csr(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Error: failed to open file %s\n", filename);
        return NULL;
    }

    int is_symmetric = 0;
    int rows = 0;
    int cols = 0;
    int entries = 0;
    if (read_header(f, &is_symmetric) != 0 || read_sizes(f, &rows, &cols, &entries) != 0) {
        fclose(f);
        return NULL;
    }
    if (rows <= 0 || rows != cols || entries < 0) {
        fprintf(stderr, "Error: invalid sizes (%d x %d, %d entries), the matrix must be square\n",
                rows, cols, entries);
        fclose(f);
        return NULL;
    }

    size_t buffer_size = (size_t)(entries > 0 ? entries : 1);
    int* src = (int*)malloc(buffer_size * sizeof(int));
    int* dst = (int*)malloc(buffer_size * sizeof(int));
    if (src == NULL || dst == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for edges\n");
        free(src);
        free(dst);
        fclose(f);
        return NULL;
    }

    CSRMatrix* csr = NULL;
    if (read_edges(f, entries, src, dst) == 0) {
        csr = csr_from_edges(rows, src, dst, entries, is_symmetric);
    }
    free(src);
    free(dst);
    fclose(f);

    if (csr != NULL) {
        printf("Graph loaded: %d vertices, %d entries in the file, %d edges in CSR (%s)\n",
               rows, entries, csr->row_ptr[rows], is_symmetric ? "undirected" : "directed");
    }
    return csr;
}

int find_max_degree_vertex(const CSRMatrix* csr) {
    if (csr == NULL || csr->n <= 0) {
        return -1;
    }

    int max_vertex = 0;
    int max_degree = -1;
    for (int v = 0; v < csr->n; v++) {
        int degree = csr->row_ptr[v + 1] - csr->row_ptr[v];
        if (degree > max_degree) {
            max_degree = degree;
            max_vertex = v;
        }
    }
    return max_vertex;
}

static int compute_depths(const int* parent, int n, int* depth, int* path) {
    for (int v = 0; v < n; v++) {
        depth[v] = (parent[v] == -1) ? -1 : DEPTH_UNKNOWN;
    }

    for (int v = 0; v < n; v++) {
        int length = 0;
        int u = v;
        while (depth[u] == DEPTH_UNKNOWN) {
            int p = parent[u];
            if (p == u) {
                depth[u] = 0;
            } else if (p < 0 || p >= n || parent[p] == -1 || length == n) {
                return -1;
            } else {
                path[length++] = u;
                u = p;
            }
        }
        while (length > 0) {
            int w = path[--length];
            depth[w] = depth[parent[w]] + 1;
        }
    }
    return 0;
}

int bfs_tree_depths(const CSRMatrix* csr, const int* parent, int* depth) {
    if (csr == NULL || parent == NULL || depth == NULL || csr->n <= 0) {
        return -1;
    }
    int n = csr->n;

    int* path = (int*)malloc((size_t)n * sizeof(int));
    char* has_tree_edge = (char*)calloc((size_t)n, sizeof(char));
    if (path == NULL || has_tree_edge == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for the BFS check\n");
        free(path);
        free(has_tree_edge);
        return -1;
    }

    int status = compute_depths(parent, n, depth, path);

    for (int u = 0; u < n && status == 0; u++) {
        for (int i = csr->row_ptr[u]; i < csr->row_ptr[u + 1]; i++) {
            int v = csr->col_idx[i];
            if (parent[v] == u && v != u) {
                has_tree_edge[v] = 1;
            }
            if (depth[u] != -1 && (depth[v] == -1 || depth[v] > depth[u] + 1)) {
                status = -1;
            }
        }
    }

    for (int v = 0; v < n && status == 0; v++) {
        if (parent[v] != -1 && parent[v] != v && !has_tree_edge[v]) {
            status = -1;
        }
    }

    free(path);
    free(has_tree_edge);
    return status;
}
