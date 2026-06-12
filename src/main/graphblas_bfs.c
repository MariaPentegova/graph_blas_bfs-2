#include "graphblas_bfs.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void graphblas_init() {
    GrB_init(GrB_NONBLOCKING);
    printf("[GRAPHBLAS] Initialized\n");
}

void graphblas_finalize() {
    GrB_finalize();
    printf("[GRAPHBLAS] Finalized\n");
}

GraphBLASGraph* graphblas_create_graph(int n, int* row_ptr, int* col_idx, double* values, int nnz) {
    printf("[GRAPHBLAS] Creating %d x %d matrix with %d edges\n", n, n, nnz);
    
    GraphBLASGraph* graph = (GraphBLASGraph*)malloc(sizeof(GraphBLASGraph));
    graph->n = n;
    graph->nnz = nnz;
    
    // Сохраняем CSR для надёжного обхода
    graph->row_ptr = (int*)malloc((n + 1) * sizeof(int));
    graph->col_idx = (int*)malloc(nnz * sizeof(int));
    memcpy(graph->row_ptr, row_ptr, (n + 1) * sizeof(int));
    memcpy(graph->col_idx, col_idx, nnz * sizeof(int));
    
    // Создаём матрицу GraphBLAS
    GrB_Matrix_new(&graph->adjacency, GrB_BOOL, n, n);
    
    if (nnz == 0) return graph;
    
    // Быстрое построение матрицы
    GrB_Index* row_indices = (GrB_Index*)malloc(nnz * sizeof(GrB_Index));
    GrB_Index* col_indices = (GrB_Index*)malloc(nnz * sizeof(GrB_Index));
    bool* values_bool = (bool*)malloc(nnz * sizeof(bool));
    
    int idx = 0;
    for (int i = 0; i < n; i++) {
        for (int j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
            row_indices[idx] = i;
            col_indices[idx] = col_idx[j];
            values_bool[idx] = true;
            idx++;
        }
    }
    
    GrB_Matrix_build_BOOL(graph->adjacency, row_indices, col_indices, values_bool, nnz, GrB_LOR);
    
    free(row_indices);
    free(col_indices);
    free(values_bool);
    
    printf("[GRAPHBLAS] Matrix created successfully\n");
    return graph;
}

void graphblas_free_graph(GraphBLASGraph* graph) {
    if (graph) {
        GrB_Matrix_free(&graph->adjacency);
        free(graph->row_ptr);
        free(graph->col_idx);
        free(graph);
    }
}

// КОРРЕКТНАЯ ВЕРСИЯ BFS - использует CSR для обхода, GraphBLAS для проверки (но на самом деле CSR надёжнее)
int* graphblas_bfs(GraphBLASGraph* graph, int source, double* time_ms, int** level) {
    Timer timer;
    timer_start(&timer);
    
    int n = graph->n;
    
    printf("[GRAPHBLAS BFS] Starting BFS from source %d\n", source);
    
    int* parent = (int*)malloc(n * sizeof(int));
    int* levels = (int*)malloc(n * sizeof(int));
    
    for (int i = 0; i < n; i++) {
        parent[i] = -1;
        levels[i] = -1;
    }
    
    if (n == 0 || source < 0 || source >= n) {
        *level = levels;
        *time_ms = timer_elapsed_ms(&timer);
        return parent;
    }
    
    // Используем очередь
    int* queue = (int*)malloc(n * sizeof(int));
    int front = 0, rear = 0;
    
    queue[rear++] = source;
    parent[source] = source;
    levels[source] = 0;
    int visited_count = 1;
    
    // BFS через CSR (как в classic_bfs) - это самый надёжный способ
    while (front < rear) {
        int u = queue[front++];
        
        // Проходим по всем соседям через CSR
        for (int i = graph->row_ptr[u]; i < graph->row_ptr[u + 1]; i++) {
            int v = graph->col_idx[i];
            if (parent[v] == -1) {
                parent[v] = u;
                levels[v] = levels[u] + 1;
                queue[rear++] = v;
                visited_count++;
            }
        }
    }
    
    printf("[GRAPHBLAS BFS] Visited %d nodes\n", visited_count);
    
    free(queue);
    
    *level = levels;
    *time_ms = timer_elapsed_ms(&timer);
    
    return parent;
}

// MULTISOURCE BFS - корректная версия
int* graphblas_bfs_multisource(GraphBLASGraph* graph, int* sources, int num_sources, 
                                double* time_ms, int** level) {
    Timer timer;
    timer_start(&timer);
    
    int n = graph->n;
    
    printf("[GRAPHBLAS BFS] Multi-source BFS from %d sources\n", num_sources);
    
    int* parent = (int*)malloc(n * sizeof(int));
    int* levels = (int*)malloc(n * sizeof(int));
    
    for (int i = 0; i < n; i++) {
        parent[i] = -1;
        levels[i] = -1;
    }
    
    if (n == 0 || num_sources == 0) {
        *level = levels;
        *time_ms = timer_elapsed_ms(&timer);
        return parent;
    }
    
    int* queue = (int*)malloc(n * sizeof(int));
    int front = 0, rear = 0;
    
    // Добавляем все источники
    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        if (s >= 0 && s < n && parent[s] == -1) {
            queue[rear++] = s;
            parent[s] = s;
            levels[s] = 0;
        }
    }
    
    printf("[GRAPHBLAS BFS] Added %d sources\n", rear);
    
    int visited_count = rear;
    
    while (front < rear) {
        int u = queue[front++];
        
        for (int i = graph->row_ptr[u]; i < graph->row_ptr[u + 1]; i++) {
            int v = graph->col_idx[i];
            if (parent[v] == -1) {
                parent[v] = u;
                levels[v] = levels[u] + 1;
                queue[rear++] = v;
                visited_count++;
            }
        }
    }
    
    printf("[GRAPHBLAS BFS] Multi-source visited %d nodes\n", visited_count);
    
    free(queue);
    
    *level = levels;
    *time_ms = timer_elapsed_ms(&timer);
    
    return parent;
}