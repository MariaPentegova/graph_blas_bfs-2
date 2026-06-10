#include "graphblas_bfs.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void graphblas_init() {
    GrB_init(GrB_NONBLOCKING);
    printf("[GRAPHBLAS] Initialized (non-blocking mode)\n");
}

void graphblas_finalize() {
    GrB_finalize();
    printf("[GRAPHBLAS] Finalized\n");
}

GraphBLASGraph* graphblas_create_graph(int n, int* row_ptr, int* col_idx, double* values, int nnz) {
    printf("[GRAPHBLAS] Creating %d x %d matrix with %d entries\n", n, n, nnz);
    
    GraphBLASGraph* graph = (GraphBLASGraph*)malloc(sizeof(GraphBLASGraph));
    graph->n = n;
    
    // Create boolean adjacency matrix (structural graph)
    GrB_Matrix_new(&graph->adjacency, GrB_BOOL, n, n);
    
    // Build from COO format
    GrB_Index* I = (GrB_Index*)malloc(nnz * sizeof(GrB_Index));
    GrB_Index* J = (GrB_Index*)malloc(nnz * sizeof(GrB_Index));
    bool* X = (bool*)malloc(nnz * sizeof(bool));
    
    int idx = 0;
    for (int i = 0; i < n; i++) {
        for (int j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
            I[idx] = i;
            J[idx] = col_idx[j];
            X[idx] = true;
            idx++;
        }
    }
    
    GrB_Matrix_build(graph->adjacency, I, J, X, nnz, GrB_LOR);
    
    free(I);
    free(J);
    free(X);
    
    printf("[GRAPHBLAS] Matrix created successfully\n");
    return graph;
}

void graphblas_free_graph(GraphBLASGraph* graph) {
    if (graph) {
        GrB_Matrix_free(&graph->adjacency);
        free(graph);
        printf("[GRAPHBLAS] Graph freed\n");
    }
}

// BFS using GraphBLAS - IMPLEMENTS THE SAME ALGORITHM AS classic_bfs.c
// Logic:
//   1. Start with frontier = {source}
//   2. While frontier not empty:
//      a. new_frontier = frontier * adjacency  (find all neighbors)
//      b. new_frontier = new_frontier - visited (remove visited nodes)
//      c. Update parent and level for new nodes
//      d. frontier = new_frontier
int* graphblas_bfs(GraphBLASGraph* graph, int source, double* time_ms, int** level) {
    printf("\n[GRAPHBLAS BFS] Starting from source node %d\n", source);
    
    Timer timer;
    timer_start(&timer);
    
    int n = graph->n;
    
    // Result arrays (same as classic BFS)
    int* parent = (int*)malloc(n * sizeof(int));
    int* levels = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        parent[i] = -1;
        levels[i] = -1;
    }
    
    // GraphBLAS vectors
    GrB_Vector frontier;      // Current frontier (nodes at current level)
    GrB_Vector visited;       // All visited nodes
    GrB_Vector new_frontier;  // Next frontier (neighbors)
    
    GrB_Vector_new(&frontier, GrB_BOOL, n);
    GrB_Vector_new(&visited, GrB_BOOL, n);
    GrB_Vector_new(&new_frontier, GrB_BOOL, n);
    
    // Initialize with source (same as queue_push in classic BFS)
    if (source >= 0 && source < n) {
        GrB_Vector_setElement(frontier, true, source);
        GrB_Vector_setElement(visited, true, source);
        parent[source] = source;
        levels[source] = 0;
    }
    
    int current_level = 0;
    int visited_count = 1;
    GrB_Index nvals;
    
    printf("[GRAPHBLAS BFS] Starting main loop\n");
    
    // Main BFS loop (same logic as while(!queue_empty(q)) in classic BFS)
    while (1) {
        // Step 1: new_frontier = frontier * adjacency
        // This is equivalent to: for each u in frontier: for each neighbor v
        GrB_vxm(new_frontier, GrB_NULL, GrB_NULL, GrB_LOR_LAND_BOOL,
                frontier, graph->adjacency, GrB_NULL);
        
        // Step 2: Remove already visited nodes
        // This is equivalent to: if (!visited[v]) in classic BFS
        GrB_eWiseMult(new_frontier, visited, GrB_NULL, GrB_LAND,
                      new_frontier, frontier, GrB_NULL);
        
        // Check if there are new nodes (equivalent to queue_empty check)
        GrB_Vector_nvals(&nvals, new_frontier);
        if (nvals == 0) {
            printf("[GRAPHBLAS BFS] No new nodes, stopping\n");
            break;
        }
        
        // Increment level
        current_level++;
        
        // Update visited (visited = visited OR new_frontier)
        GrB_eWiseAdd(visited, GrB_NULL, GrB_NULL, GrB_LOR,
                     visited, new_frontier, GrB_NULL);
        
        // Update parent and level for new nodes
        // This is equivalent to the inner loop in classic BFS where we set parent[v] = u
        for (int i = 0; i < n; i++) {
            bool in_new_frontier;
            GrB_Vector_extractElement(&in_new_frontier, new_frontier, i);
            
            if (in_new_frontier && parent[i] == -1) {
                // Find the parent - one of the nodes in current frontier that has edge to i
                for (int j = 0; j < n; j++) {
                    bool in_frontier;
                    GrB_Vector_extractElement(&in_frontier, frontier, j);
                    
                    if (in_frontier) {
                        // Check if edge exists (j -> i)
                        bool has_edge;
                        GrB_Matrix_extractElement(&has_edge, graph->adjacency, j, i);
                        if (has_edge) {
                            parent[i] = j;
                            levels[i] = current_level;
                            visited_count++;
                            break;
                        }
                    }
                }
            }
        }
        
        // Update frontier for next iteration (same as queue_push for all new nodes)
        GrB_Vector_clear(frontier);
        GrB_Vector_assign(frontier, GrB_NULL, GrB_NULL, new_frontier, GrB_ALL, n, GrB_NULL);
        
        printf("[GRAPHBLAS BFS] Level %d: added %d nodes (total visited: %d)\n", 
               current_level, (int)nvals, visited_count);
    }
    
    printf("[GRAPHBLAS BFS] Completed. Visited %d/%d nodes\n", visited_count, n);
    
    // Cleanup
    GrB_Vector_free(&frontier);
    GrB_Vector_free(&visited);
    GrB_Vector_free(&new_frontier);
    
    *level = levels;
    *time_ms = timer_elapsed_ms(&timer);
    
    return parent;
}

// Multi-source BFS using GraphBLAS
int* graphblas_bfs_multisource(GraphBLASGraph* graph, int* sources, int num_sources, 
                                double* time_ms, int** level) {
    printf("\n[GRAPHBLAS BFS] Multi-source BFS from %d sources\n", num_sources);
    
    Timer timer;
    timer_start(&timer);
    
    int n = graph->n;
    
    int* parent = (int*)malloc(n * sizeof(int));
    int* levels = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        parent[i] = -1;
        levels[i] = -1;
    }
    
    GrB_Vector frontier, visited, new_frontier;
    GrB_Vector_new(&frontier, GrB_BOOL, n);
    GrB_Vector_new(&visited, GrB_BOOL, n);
    GrB_Vector_new(&new_frontier, GrB_BOOL, n);
    
    // Initialize with all sources
    int sources_added = 0;
    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        if (s >= 0 && s < n) {
            GrB_Vector_setElement(frontier, true, s);
            GrB_Vector_setElement(visited, true, s);
            parent[s] = s;
            levels[s] = 0;
            sources_added++;
        }
    }
    
    printf("[GRAPHBLAS BFS] Added %d sources\n", sources_added);
    
    int current_level = 0;
    int visited_count = sources_added;
    GrB_Index nvals;
    
    // Main BFS loop
    while (1) {
        // Find all neighbors of current frontier
        GrB_vxm(new_frontier, GrB_NULL, GrB_NULL, GrB_LOR_LAND_BOOL,
                frontier, graph->adjacency, GrB_NULL);
        
        // Remove already visited nodes
        GrB_eWiseMult(new_frontier, visited, GrB_NULL, GrB_LAND,
                      new_frontier, frontier, GrB_NULL);
        
        GrB_Vector_nvals(&nvals, new_frontier);
        if (nvals == 0) break;
        
        current_level++;
        
        // Update visited
        GrB_eWiseAdd(visited, GrB_NULL, GrB_NULL, GrB_LOR,
                     visited, new_frontier, GrB_NULL);
        
        // Update parent and level for new nodes
        for (int i = 0; i < n; i++) {
            bool in_new_frontier;
            GrB_Vector_extractElement(&in_new_frontier, new_frontier, i);
            
            if (in_new_frontier && parent[i] == -1) {
                for (int j = 0; j < n; j++) {
                    bool in_frontier;
                    GrB_Vector_extractElement(&in_frontier, frontier, j);
                    
                    if (in_frontier) {
                        bool has_edge;
                        GrB_Matrix_extractElement(&has_edge, graph->adjacency, j, i);
                        if (has_edge) {
                            parent[i] = j;
                            levels[i] = current_level;
                            visited_count++;
                            break;
                        }
                    }
                }
            }
        }
        
        // Update frontier
        GrB_Vector_clear(frontier);
        GrB_Vector_assign(frontier, GrB_NULL, GrB_NULL, new_frontier, GrB_ALL, n, GrB_NULL);
    }
    
    printf("[GRAPHBLAS BFS] Multi-source completed. Visited %d/%d nodes\n", 
           visited_count, n);
    
    GrB_Vector_free(&frontier);
    GrB_Vector_free(&visited);
    GrB_Vector_free(&new_frontier);
    
    *level = levels;
    *time_ms = timer_elapsed_ms(&timer);
    
    return parent;
}
