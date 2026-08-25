#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  
#include <GraphBLAS.h>
#include "utils.h"
#include "graphblas_bfs.h"

void lor_bool(void* z, const void* x, const void* y) {
    bool a = *(const bool*)x;
    bool b = *(const bool*)y;
    *(bool*)z = a || b;
}

void land_bool(void* z, const void* x, const void* y) {
    bool a = *(const bool*)x;
    bool b = *(const bool*)y;
    *(bool*)z = a && b;
}

int csr_to_graphblas(CSRMatrix* csr, void** A) {
    if (csr == NULL || A == NULL) {
        return -1;
    }
    
    // Приводим void** к GrB_Matrix*
    GrB_Matrix* mat = (GrB_Matrix*)A;
    
    int n = csr->n;
    int total_edges = csr->row_ptr[n];
    
    GrB_Index* row_ptr = (GrB_Index*)csr->row_ptr;
    GrB_Index* col_idx = (GrB_Index*)csr->col_idx;
    
    GrB_Info info = GxB_Matrix_import_CSR(
        mat,
        GrB_BOOL,
        n,
        n,
        &row_ptr,
        &col_idx,
        NULL,
        n + 1,
        total_edges,
        0,
        true,      // ← вместо GxB_TRUE
        NULL,
        NULL
    );
    
    return (info == GrB_SUCCESS) ? 0 : -1;
}

int graphblas_level_bfs(CSRMatrix* csr, int start_vertex, int* level) {
    if (csr == NULL || level == NULL || start_vertex < 0 || start_vertex >= csr->n) {
        return -1;
    }
    
    int n = csr->n;
    GrB_Matrix A = NULL;
    
    // Явное приведение типа!
    if (csr_to_graphblas(csr, (void**)&A) != 0) {
        printf("Error: не удалось превратить CSR в GraphBLAS\n");
        return -1;
    }
    
    GrB_Vector level_vec = NULL;
    GrB_Vector frontier = NULL;
    GrB_Vector visited = NULL;
    GrB_Vector new_frontier = NULL;
    
    GrB_Vector_new(&level_vec, GrB_INT32, n);
    GrB_Vector_new(&frontier, GrB_BOOL, n);
    GrB_Vector_new(&visited, GrB_BOOL, n);
    GrB_Vector_new(&new_frontier, GrB_BOOL, n);
    
    GrB_Vector_setElement_INT32(level_vec, 0, start_vertex);
    GrB_Vector_setElement_BOOL(frontier, 1, start_vertex);
    GrB_Vector_setElement_BOOL(visited, 1, start_vertex);
    
    for (int i = 0; i < n; i++) {
        level[i] = -1;
    }
    level[start_vertex] = 0;
    
    // Создаём семиринг вручную
    GrB_BinaryOp lor_op, land_op;
    GrB_BinaryOp_new(&lor_op, lor_bool, GrB_BOOL, GrB_BOOL, GrB_BOOL);
    GrB_BinaryOp_new(&land_op, land_bool, GrB_BOOL, GrB_BOOL, GrB_BOOL);
    
    GrB_Monoid monoid;
    GrB_Monoid_new(&monoid, lor_op, (bool)0);
    
    GrB_Semiring semiring;
    GrB_Semiring_new(&semiring, monoid, land_op);
    
    int current_level = 0;
    GrB_Index nvals = 1;
    
    while (nvals > 0) {
        GrB_mxv(
            new_frontier,
            visited,
            NULL,
            semiring,
            A,
            frontier,
            NULL
        );
        
        GrB_eWiseAdd(
            visited,
            NULL,
            NULL,
            lor_op,   
            visited,
            new_frontier,
            NULL
        );
        
        GrB_assign(
            level_vec,
            new_frontier,
            NULL,
            current_level + 1,
            GrB_ALL,
            n,
            NULL
        );
        
        GrB_Vector_clear(frontier);
        GrB_assign(frontier, NULL, NULL, new_frontier, GrB_ALL, n, NULL);
        
        GrB_Vector_nvals(&nvals, new_frontier);
        current_level++;
    }
    
    for (int i = 0; i < n; i++) {
        GrB_Vector_extractElement_INT32(&level[i], level_vec, i);
    }
    
    GrB_Semiring_free(&semiring);
    GrB_Monoid_free(&monoid);
    GrB_BinaryOp_free(&lor_op);
    GrB_BinaryOp_free(&land_op);
    GrB_Matrix_free(&A);
    GrB_Vector_free(&level_vec);
    GrB_Vector_free(&frontier);
    GrB_Vector_free(&visited);
    GrB_Vector_free(&new_frontier);
    
    return 0;
}

int graphblas_multisource_level_bfs(CSRMatrix* csr, int* sources, int num_sources, int* level) {
    if (csr == NULL || level == NULL || sources == NULL || num_sources <= 0) {
        return -1;
    }
    
    int n = csr->n;
    GrB_Matrix A = NULL;
    
    // Явное приведение типа!
    if (csr_to_graphblas(csr, (void**)&A) != 0) {
        printf("Error: не удалось импортировать CSR в GraphBLAS\n");
        return -1;
    }
    
    GrB_Vector level_vec = NULL;
    GrB_Vector frontier = NULL;
    GrB_Vector visited = NULL;
    GrB_Vector new_frontier = NULL;
    
    GrB_Vector_new(&level_vec, GrB_INT32, n);
    GrB_Vector_new(&frontier, GrB_BOOL, n);
    GrB_Vector_new(&visited, GrB_BOOL, n);
    GrB_Vector_new(&new_frontier, GrB_BOOL, n);
    
    for (int i = 0; i < n; i++) {
        level[i] = -1;
    }
    
    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        if (s >= 0 && s < n) {
            GrB_Vector_setElement_INT32(level_vec, 0, s);
            GrB_Vector_setElement_BOOL(frontier, 1, s);
            GrB_Vector_setElement_BOOL(visited, 1, s);
            level[s] = 0;
        }
    }
    
    // Создаём семиринг вручную
    GrB_BinaryOp lor_op, land_op;
    GrB_BinaryOp_new(&lor_op, lor_bool, GrB_BOOL, GrB_BOOL, GrB_BOOL);
    GrB_BinaryOp_new(&land_op, land_bool, GrB_BOOL, GrB_BOOL, GrB_BOOL);
    
    GrB_Monoid monoid;
    GrB_Monoid_new(&monoid, lor_op, (bool)0);
    
    GrB_Semiring semiring;
    GrB_Semiring_new(&semiring, monoid, land_op);
    
    int current_level = 0;
    GrB_Index nvals = num_sources;
    
    while (nvals > 0) {
        GrB_mxv(
            new_frontier,
            visited,
            NULL,
            semiring,
            A,
            frontier,
            NULL
        );
        
        GrB_eWiseAdd(
            visited,
            NULL,
            NULL,
            lor_op,
            visited,
            new_frontier,
            NULL
        );
        
        GrB_assign(
            level_vec,
            new_frontier,
            NULL,
            current_level + 1,
            GrB_ALL,
            n,
            NULL
        );
        
        GrB_Vector_clear(frontier);
        GrB_assign(frontier, NULL, NULL, new_frontier, GrB_ALL, n, NULL);
        
        GrB_Vector_nvals(&nvals, new_frontier);
        current_level++;
    }
    
    for (int i = 0; i < n; i++) {
        GrB_Vector_extractElement_INT32(&level[i], level_vec, i);
    }
    
    GrB_Semiring_free(&semiring);
    GrB_Monoid_free(&monoid);
    GrB_BinaryOp_free(&lor_op);
    GrB_BinaryOp_free(&land_op);
    GrB_Matrix_free(&A);
    GrB_Vector_free(&level_vec);
    GrB_Vector_free(&frontier);
    GrB_Vector_free(&visited);
    GrB_Vector_free(&new_frontier);
    
    return 0;
}
