#include <stdio.h>
#include <stdlib.h>

#include "graphblas_bfs.h"
#include "utils.h"

// CSR в GraphBLAS
int csr_to_graphblas(CSRMatrix* csr, GrB_Matrix* A) {
    if (csr == NULL || A == NULL) {
        return -1;
    }
    
    int total_edges = csr->row_ptr[csr->n];
    
    GrB_Info info = GxB_Matrix_import_CSR(
        A,
        GrB_BOOL,
        csr->n,
        csr->n,
        &csr->row_ptr,
        &csr->col_idx,
        NULL,
        csr->n + 1,
        total_edges,
        0,
        GrB_TRUE,
        NULL
    );
    
    return (info == GrB_SUCCESS) ? 0 : -1;
}

// Level BFS на GraphBLAS
int graphblas_level_bfs(CSRMatrix* csr, int start_vertex, int* level) {
    if (csr == NULL || level == NULL || start_vertex < 0 || start_vertex >= csr->n) {
        return -1;
    }
    
    int n = csr->n;
    GrB_Matrix A = NULL;
    if (csr_to_graphblas(csr, &A) != 0) {
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
    
    int current_level = 0;
    GrB_Index nvals = 1;
    
    while (nvals > 0) {
        GrB_mxv(
            new_frontier,                
            visited,                      
            NULL,                         
            GrB_LOR_LAND_BOOL_SEMIRING,  
            A,                            
            frontier,                     
            NULL                          
        );
        
        GrB_eWiseAdd(
            visited,
            NULL,
            NULL,
            GrB_LOR_BOOL,
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
    
    GrB_Matrix_free(&A);
    GrB_Vector_free(&level_vec);
    GrB_Vector_free(&frontier);
    GrB_Vector_free(&visited);
    GrB_Vector_free(&new_frontier);
    
    return 0;
}

// Multisource Level BFS на GraphBLAS
int graphblas_multisource_level_bfs(CSRMatrix* csr, int* sources, int num_sources, int* level) {
    if (csr == NULL || level == NULL || sources == NULL || num_sources <= 0) {
        return -1;
    }
    
    int n = csr->n;
    
    GrB_Matrix A = NULL;
    if (csr_to_graphblas(csr, &A) != 0) {
        printf("Errro: не удалось импортировать CSR в GraphBLAS\n");
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
    
    int current_level = 0;
    GrB_Index nvals = num_sources;  
    
    while (nvals > 0) {
        GrB_mxv(
            new_frontier,                 
            visited,                      
            NULL,                         
            GrB_LOR_LAND_BOOL_SEMIRING,   
            A,                            
            frontier,                     
            NULL                          
        );
        
        GrB_eWiseAdd(
            visited,
            NULL,
            NULL,
            GrB_LOR_BOOL,
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
    
    GrB_Matrix_free(&A);
    GrB_Vector_free(&level_vec);
    GrB_Vector_free(&frontier);
    GrB_Vector_free(&visited);
    GrB_Vector_free(&new_frontier);
    
    return 0;
}
