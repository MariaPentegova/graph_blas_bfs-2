#include "classic_bfs.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    int* data;
    int front;
    int rear;
    int size;
    int capacity;
} Queue;

static Queue* create_queue(int capacity) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->data = (int*)malloc(capacity * sizeof(int));
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->capacity = capacity;
    return q;
}

static void queue_push(Queue* q, int value) {
    if (q->size < q->capacity) {
        q->rear = (q->rear + 1) % q->capacity;
        q->data[q->rear] = value;
        q->size++;
    }
}

static int queue_pop(Queue* q) {
    if (q->size > 0) {
        int value = q->data[q->front];
        q->front = (q->front + 1) % q->capacity;
        q->size--;
        return value;
    }
    return -1;
}

static int queue_empty(Queue* q) {
    return q->size == 0;
}

static void free_queue(Queue* q) {
    if (q) {
        if (q->data) free(q->data);
        free(q);
    }
}

// Single source BFS (только для связного графа или для компоненты)
BFSResult* classic_bfs(int n, int* row_ptr, int* col_idx, int source) {
    printf("[CLASSIC BFS] Starting from source %d\n", source);
    
    BFSResult* result = (BFSResult*)malloc(sizeof(BFSResult));
    if (!result) return NULL;
    
    result->parent = (int*)malloc(n * sizeof(int));
    result->level = (int*)malloc(n * sizeof(int));
    result->visited = (int*)calloc(n, sizeof(int));
    result->visited_count = 0;
    
    if (!result->parent || !result->level || !result->visited) {
        if (result->parent) free(result->parent);
        if (result->level) free(result->level);
        if (result->visited) free(result->visited);
        free(result);
        return NULL;
    }
    
    for (int i = 0; i < n; i++) {
        result->parent[i] = -1;
        result->level[i] = -1;
    }
    
    if (source < 0 || source >= n) {
        return result;
    }
    
    Queue* q = create_queue(n);
    if (!q) return result;
    
    result->visited[source] = 1;
    result->level[source] = 0;
    result->parent[source] = source;
    result->visited_count = 1;
    queue_push(q, source);
    
    while (!queue_empty(q)) {
        int u = queue_pop(q);
        
        for (int i = row_ptr[u]; i < row_ptr[u + 1]; i++) {
            int v = col_idx[i];
            if (!result->visited[v]) {
                result->visited[v] = 1;
                result->level[v] = result->level[u] + 1;
                result->parent[v] = u;
                result->visited_count++;
                queue_push(q, v);
            }
        }
    }
    
    printf("[CLASSIC BFS] Visited %d nodes (component size)\n", result->visited_count);
    
    free_queue(q);
    return result;
}

// Multi-source BFS - работает даже на несвязном графе
// Каждый источник начинает свою компоненту связности
void classic_bfs_multisource(int n, int* row_ptr, int* col_idx, 
                              int* sources, int num_sources, BFSResult* result) {
    printf("[CLASSIC BFS] Multi-source from %d sources\n", num_sources);
    
    result->parent = (int*)malloc(n * sizeof(int));
    result->level = (int*)malloc(n * sizeof(int));
    result->visited = (int*)calloc(n, sizeof(int));
    result->visited_count = 0;
    
    for (int i = 0; i < n; i++) {
        result->parent[i] = -1;
        result->level[i] = -1;
    }
    
    Queue* q = create_queue(n);
    
    // Инициализация всеми источниками (каждый из своей компоненты)
    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        if (s >= 0 && s < n && !result->visited[s]) {
            result->visited[s] = 1;
            result->level[s] = 0;
            result->parent[s] = s;
            result->visited_count++;
            queue_push(q, s);
            printf("[CLASSIC BFS] Added source %d (component %d)\n", s, i);
        }
    }
    
    // BFS обходит все компоненты одновременно
    while (!queue_empty(q)) {
        int u = queue_pop(q);
        
        for (int i = row_ptr[u]; i < row_ptr[u + 1]; i++) {
            int v = col_idx[i];
            if (!result->visited[v]) {
                result->visited[v] = 1;
                result->level[v] = result->level[u] + 1;
                result->parent[v] = u;
                result->visited_count++;
                queue_push(q, v);
            }
        }
    }
    
    printf("[CLASSIC BFS] Multi-source visited %d nodes across all components\n", result->visited_count);
    
    free_queue(q);
}

void free_bfs_result(BFSResult* result) {
    if (result) {
        if (result->parent) free(result->parent);
        if (result->level) free(result->level);
        if (result->visited) free(result->visited);
        free(result);
    }
}