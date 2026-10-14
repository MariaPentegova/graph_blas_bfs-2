#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <GraphBLAS.h>
#include "graphblas_bfs.h"

int graphblas_init(void) {
    GrB_Info info = GrB_init(GrB_NONBLOCKING);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "GrB_init failed with error (%d)\n", (int)info);
        return -1;
    }
    printf("GraphBLAS initialized\n");
    return 0;
}

void graphblas_finalize(void) {
    GrB_finalize();
}

int graphblas_num_threads(void) {
    int num_threads = 0;
    if (GxB_Global_Option_get(GxB_NTHREADS, &num_threads) != GrB_SUCCESS) {
        return -1;
    }
    return num_threads;
}

int graphblas_build_matrix(const CSRMatrix* csr, void** A_out) {
    if (csr == NULL || A_out == NULL) {
        return -1;
    }

    GrB_Index n = (GrB_Index)csr->n;
    GrB_Index total_edges = (GrB_Index)csr->row_ptr[csr->n];
    GrB_Index col_count = total_edges > 0 ? total_edges : 1;

    GrB_Index* row_ptr = (GrB_Index*)malloc((n + 1) * sizeof(GrB_Index));
    GrB_Index* col_idx = (GrB_Index*)malloc(col_count * sizeof(GrB_Index));
    bool* value = (bool*)malloc(sizeof(bool));
    if (row_ptr == NULL || col_idx == NULL || value == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for the GraphBLAS matrix\n");
        free(row_ptr);
        free(col_idx);
        free(value);
        return -1;
    }
    for (GrB_Index i = 0; i <= n; i++) {
        row_ptr[i] = (GrB_Index)csr->row_ptr[i];
    }
    for (GrB_Index i = 0; i < total_edges; i++) {
        col_idx[i] = (GrB_Index)csr->col_idx[i];
    }
    value[0] = true;

    GrB_Matrix* A = (GrB_Matrix*)A_out;
    void* values_ptr = value;
    GrB_Info info = GxB_Matrix_import_CSR(
        A, GrB_BOOL, n, n,
        &row_ptr, &col_idx, &values_ptr,
        (n + 1) * sizeof(GrB_Index), col_count * sizeof(GrB_Index), sizeof(bool),
        true,
        false,
        NULL
    );
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "GxB_Matrix_import_CSR failed with error (%d)\n", (int)info);
        free(row_ptr);
        free(col_idx);
        free(value);
        return -1;
    }
    return 0;
}

void graphblas_free_matrix(void* A_handle) {
    if (A_handle == NULL) {
        return;
    }
    GrB_Matrix A = (GrB_Matrix)A_handle;
    GrB_Matrix_free(&A);
}

static GrB_Info set_sources(GrB_Vector parent_vec, GrB_Vector frontier, int n,
                            const int* sources, int num_sources) {
    GrB_Info info = GrB_SUCCESS;
    for (int i = 0; i < num_sources && info == GrB_SUCCESS; i++) {
        int s = sources[i];
        if (s >= 0 && s < n) {
            info = GrB_Vector_setElement_INT32(parent_vec, s, (GrB_Index)s);
            if (info == GrB_SUCCESS) {
                info = GrB_Vector_setElement_INT32(frontier, s, (GrB_Index)s);
            }
        }
    }
    return info;
}

static GrB_Info bfs_loop(GrB_Matrix A, int n, GrB_Vector parent_vec, GrB_Vector frontier) {
    GrB_Index frontier_size = 0;
    GrB_Info info = GrB_Vector_nvals(&frontier_size, frontier);

    while (info == GrB_SUCCESS && frontier_size > 0) {
        info = GrB_vxm(frontier, parent_vec, NULL, GxB_ANY_SECONDI_INT32,
                       frontier, A, GrB_DESC_RSC);
        if (info == GrB_SUCCESS) {
            info = GrB_Vector_assign(parent_vec, frontier, NULL, frontier,
                                     GrB_ALL, (GrB_Index)n, GrB_DESC_S);
        }
        if (info == GrB_SUCCESS) {
            info = GrB_Vector_nvals(&frontier_size, frontier);
        }
    }
    return info;
}

static int run_bfs(GrB_Matrix A, int n, const int* sources, int num_sources, void** parent_out) {
    GrB_Vector parent_vec = NULL;
    GrB_Vector frontier = NULL;

    GrB_Info info = GrB_Vector_new(&parent_vec, GrB_INT32, (GrB_Index)n);
    if (info == GrB_SUCCESS) {
        info = GrB_Vector_new(&frontier, GrB_INT32, (GrB_Index)n);
    }
    if (info == GrB_SUCCESS) {
        info = set_sources(parent_vec, frontier, n, sources, num_sources);
    }
    if (info == GrB_SUCCESS) {
        info = bfs_loop(A, n, parent_vec, frontier);
    }
    if (info == GrB_SUCCESS) {
        info = GrB_Vector_wait(parent_vec, GrB_MATERIALIZE);
    }

    GrB_Vector_free(&frontier);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "GraphBLAS BFS failed with error (%d)\n", (int)info);
        GrB_Vector_free(&parent_vec);
        return -1;
    }
    *parent_out = parent_vec;
    return 0;
}

int graphblas_parent_bfs(const CSRMatrix* csr, void* A_handle, int start_vertex,
                         void** parent_out) {
    if (parent_out != NULL) {
        *parent_out = NULL;
    }
    if (csr == NULL || A_handle == NULL || parent_out == NULL ||
        start_vertex < 0 || start_vertex >= csr->n) {
        return -1;
    }
    return run_bfs((GrB_Matrix)A_handle, csr->n, &start_vertex, 1, parent_out);
}

int graphblas_multisource_parent_bfs(const CSRMatrix* csr, void* A_handle,
                                     const int* sources, int num_sources, void** parent_out) {
    if (parent_out != NULL) {
        *parent_out = NULL;
    }
    if (csr == NULL || A_handle == NULL || parent_out == NULL ||
        sources == NULL || num_sources <= 0) {
        return -1;
    }
    return run_bfs((GrB_Matrix)A_handle, csr->n, sources, num_sources, parent_out);
}

int graphblas_parent_to_array(void* parent_handle, int n, int* parent) {
    if (parent_handle == NULL || parent == NULL) {
        return -1;
    }
    GrB_Vector parent_vec = (GrB_Vector)parent_handle;

    GrB_Index count = 0;
    if (GrB_Vector_nvals(&count, parent_vec) != GrB_SUCCESS) {
        return -1;
    }

    GrB_Index buffer_size = count > 0 ? count : 1;
    GrB_Index* indices = (GrB_Index*)malloc(buffer_size * sizeof(GrB_Index));
    int32_t* values = (int32_t*)malloc(buffer_size * sizeof(int32_t));
    if (indices == NULL || values == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for the BFS result\n");
        free(indices);
        free(values);
        return -1;
    }

    int status = -1;
    if (GrB_Vector_extractTuples_INT32(indices, values, &count, parent_vec) == GrB_SUCCESS) {
        for (int i = 0; i < n; i++) {
            parent[i] = -1;
        }
        for (GrB_Index k = 0; k < count; k++) {
            parent[indices[k]] = (int)values[k];
        }
        status = 0;
    }

    free(indices);
    free(values);
    return status;
}

void graphblas_free_vector(void* vector_handle) {
    if (vector_handle == NULL) {
        return;
    }
    GrB_Vector v = (GrB_Vector)vector_handle;
    GrB_Vector_free(&v);
}
