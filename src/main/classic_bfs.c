#include "classic_bfs.h"
#include <stdlib.h>
#include <stdio.h>

// Simple queue implementation for BFS
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
    free(q->data);
    free(q);
}

BFSResult* classic_bfs(int n, int* row_ptr, int* col_idx, int source) {
    printf("\n[CLASSIC BFS] Starting from source node %d\n", source);
    
    BFSResult* result = (BFSResult*)malloc(sizeof(BFSResult));
    result->parent = (int*)malloc(n * sizeof(int));
    result->level = (int*)malloc(n * sizeof(int));
    result->visited = (int*)calloc(n, sizeof(int));
    result->visited_count = 0;
    
    // Initialize arrays
    for (int i = 0; i < n; i++) {
        result->parent[i] = -1;
        result->level[i] = -1;
    }
    
    // Validate source
    if (source < 0 || source >= n) {
        printf("[CLASSIC BFS] Error: Invalid source node %d\n", source);
        return result;
    }
    
    // Initialize queue with source
    Queue* q = create_queue(n);
    result->visited[source] = 1;
    result->level[source] = 0;
    result->parent[source] = source;
    result->visited_count = 1;
    queue_push(q, source);
    
    printf("[CLASSIC BFS] Queue initialized with source node %d\n", source);
    
    // BFS main loop
    while (!queue_empty(q)) {
        int u = queue_pop(q);
        
        // Explore all neighbors of u
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
    
    printf("[CLASSIC BFS] Completed. Visited %d/%d nodes\n", 
           result->visited_count, n);
    
    free_queue(q);
    return result;
}

void classic_bfs_multisource(int n, int* row_ptr, int* col_idx, 
                              int* sources, int num_sources, BFSResult* result) {
    printf("\n[CLASSIC BFS] Multi-source BFS from %d sources\n", num_sources);
    
    result->parent = (int*)malloc(n * sizeof(int));
    result->level = (int*)malloc(n * sizeof(int));
    result->visited = (int*)calloc(n, sizeof(int));
    result->visited_count = 0;
    
    for (int i = 0; i < n; i++) {
        result->parent[i] = -1;
        result->level[i] = -1;
    }
    
    Queue* q = create_queue(n);
    
    // Initialize queue with all sources
    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        if (s >= 0 && s < n && !result->visited[s]) {
            result->visited[s] = 1;
            result->level[s] = 0;
            result->parent[s] = s;
            result->visited_count++;
            queue_push(q, s);
            printf("[CLASSIC BFS] Added source: %d\n", s);
        }
    }
    
    // BFS main loop
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
    
    printf("[CLASSIC BFS] Multi-source BFS completed. Visited %d/%d nodes\n", 
           result->visited_count, n);
    
    free_queue(q);
}

void free_bfs_result(BFSResult* result) {
    if (result) {
        free(result->parent);
        free(result->level);
        free(result->visited);
        free(result);
    }
}
