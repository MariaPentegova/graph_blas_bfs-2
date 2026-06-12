#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "classic_bfs.h"
#include "graphblas_bfs.h"
#include "utils.h"

void print_usage() {
    printf("\nUsage: ./bfs_analysis <graph_file.mtx> [source_node]\n");
    printf("Example: ./bfs_analysis ../graphs/bcspwr01.mtx 0\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    const char* graph_file = argv[1];
    int source = (argc > 2) ? atoi(argv[2]) : 0;
    
    printf("Graph file: %s\n", graph_file);
    printf("Source node: %d\n", source);
    
    int* row_ptr = NULL;
    int* col_idx = NULL;
    double* values = NULL;
    int n = 0, nnz = 0;
    
    if (load_matrix_market(graph_file, &row_ptr, &col_idx, &values, &n, &nnz) != 0) {
        fprintf(stderr, "Error: Failed to load graph file\n");
        return 1;
    }
    
    printf("Graph loaded: %d nodes, %d edges\n", n, nnz);
    
    // Classic BFS с правильным измерением
    printf("\nClassic BFS\n");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    BFSResult* classic_result = classic_bfs(n, row_ptr, col_idx, source);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double classic_time_us = (end.tv_sec - start.tv_sec) * 1000000.0 + 
                              (end.tv_nsec - start.tv_nsec) / 1000.0;
    printf("Time: %.0f us (%.3f ms)\n", classic_time_us, classic_time_us / 1000.0);
    
    save_bfs_result("classic_bfs_result.txt", classic_result->parent, classic_result->level, 
                    n, classic_time_us / 1000.0, "Classic BFS");
    
    // GraphBLAS BFS
    printf("\nGraphBLAS BFS\n");
    graphblas_init();
    GraphBLASGraph* gb_graph = graphblas_create_graph(n, row_ptr, col_idx, values, nnz);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    int* gb_level = NULL;
    double gb_time_ms;
    int* gb_parent = graphblas_bfs(gb_graph, source, &gb_time_ms, &gb_level);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double gb_time_us = (end.tv_sec - start.tv_sec) * 1000000.0 + 
                         (end.tv_nsec - start.tv_nsec) / 1000.0;
    printf("Time: %.0f us (%.3f ms)\n", gb_time_us, gb_time_us / 1000.0);
    
    save_bfs_result("graphblas_bfs_result.txt", gb_parent, gb_level, 
                    n, gb_time_us / 1000.0, "GraphBLAS BFS");
    
    // Performance Summary
    printf("\nPerformance Summary\n");
    printf("Classic BFS:    %.0f us (%.3f ms)\n", classic_time_us, classic_time_us / 1000.0);
    printf("GraphBLAS BFS:  %.0f us (%.3f ms)\n", gb_time_us, gb_time_us / 1000.0);
    
    if (classic_time_us < gb_time_us) {
        printf("Classic BFS is %.2fx faster\n", gb_time_us / classic_time_us);
    } else {
        printf("GraphBLAS BFS is %.2fx faster\n", classic_time_us / gb_time_us);
    }
    
    // Check consistency
    int consistent = 1;
    int reachable_classic = 0, reachable_gb = 0;
    for (int i = 0; i < n; i++) {
        if (classic_result->parent[i] != -1) reachable_classic++;
        if (gb_parent[i] != -1) reachable_gb++;
    }
    
    printf("Reachable nodes - Classic: %d, GraphBLAS: %d\n", reachable_classic, reachable_gb);
    printf("Results consistent: %s\n", (reachable_classic == reachable_gb) ? "YES" : "NO");
    
    // Save comparison
    FILE* f = fopen("bfs_comparison.txt", "w");
    if (f) {
        fprintf(f, "Classic BFS time: %.0f us (%.3f ms)\n", classic_time_us, classic_time_us / 1000.0);
        fprintf(f, "GraphBLAS BFS time: %.0f us (%.3f ms)\n", gb_time_us, gb_time_us / 1000.0);
        fprintf(f, "Speedup: %.2fx\n", gb_time_us / classic_time_us);
        fprintf(f, "Reachable nodes - Classic: %d, GraphBLAS: %d\n", reachable_classic, reachable_gb);
        fclose(f);
    }
    
    // Cleanup
    free_bfs_result(classic_result);
    free(gb_parent);
    if (gb_level) free(gb_level);
    graphblas_free_graph(gb_graph);
    graphblas_finalize();
    free(row_ptr);
    free(col_idx);
    if (values) free(values);
    
    return 0;
}