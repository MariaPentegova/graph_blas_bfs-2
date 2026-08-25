#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "classic_bfs.h"

void csr_parent_bfs(CSRMatrix* csr, int start_vertex, int* parent) {
    if (csr == NULL || parent == NULL) {
        return;
    }    
    int n = csr->n;    
    for (int i = 0; i < n; i++) {
        parent[i] = -1; 
    }
    parent[start_vertex] = start_vertex;  // корень указывает на себя
    
    int* queue = (int*)malloc(n * sizeof(int));
    if (queue == NULL) {
        printf("Error: не удалось выделить память для очереди\n");
        return;
    }
    
    int head = 0;  
    int tail = 0; 
    queue[tail++] = start_vertex;
    
    while (head < tail) {
        int v = queue[head++]; 
        
        int start = csr->row_ptr[v];
        int end = csr->row_ptr[v + 1];
        
        for (int i = start; i < end; i++) {
            int neighbor = csr->col_idx[i];
            
            // Если neighbor не посещена
            if (parent[neighbor] == -1) {
                parent[neighbor] = v;       
                queue[tail++] = neighbor; 
            }
        }
    }
    
    free(queue);
}

void csr_multisource_bfs(CSRMatrix* csr, int* sources, int num_sources, int* parent) {
    if (csr == NULL || parent == NULL || sources == NULL || num_sources == 0) {
        return;
    }
    
    int n = csr->n;
    for (int i = 0; i < n; i++) {
        parent[i] = -1;  
    }
    
    int* queue = (int*)malloc(n * sizeof(int));
    if (queue == NULL) {
        printf("Error: не удалось выделить память для очереди\n");
        return;
    }
    
    int head = 0;
    int tail = 0;
    
    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        if (s >= 0 && s < n) {
            parent[s] = s;          
            queue[tail++] = s;    
        }
    }
    
    while (head < tail) {
        int v = queue[head++];  
        int start = csr->row_ptr[v];
        int end = csr->row_ptr[v + 1];
        
        for (int i = start; i < end; i++) {
            int neighbor = csr->col_idx[i];
            
            if (parent[neighbor] == -1) {
                parent[neighbor] = v;      
                queue[tail++] = neighbor;   
            }
        }
    }
    
    free(queue);
}
