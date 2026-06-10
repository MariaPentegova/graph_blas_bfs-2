#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "classic_bfs.h"
#include "graphblas_bfs.h"
#include "utils.h"

void print_banner() {
    printf("\n");
    printf("     Graph BFS Performance Analysis: Classic vs GraphBLAS     \n");
}

void print_usage() {
    printf("\nUsage: ./bfs_analysis <graph_file.mtx> [source_node]\n");
    printf("\nExamples:\n");
    printf("  ./bfs_analysis ../graphs/bcspwr01.mtx 0\n");
    printf("  ./bfs_analysis ../graphs/can_96.mtx 42\n");
    printf("\nSupported formats:\n");
    printf("  - Matrix Market (.mtx) with 'pattern' or 'real' values\n");
    printf("  - Symmetric and general matrices\n");
    printf("\nOutput files:\n");
    printf("  - classic_bfs_result.txt    (BFS tree from classic algorithm)\n");
    printf("  - graphblas_bfs_result.txt  (BFS tree from GraphBLAS algorithm)\n");
    printf("  - bfs_comparison.txt        (Performance comparison)\n\n");
}

int verify_results(BFSResult* classic, int* gb_parent, int* gb_level, int n) {
    printf("\n[VERIFICATION] Checking consistency between algorithms...\n");
    
    int consistent = 1;
    int mismatches = 0;
    
    for (int i = 0; i < n; i++) {
        int classic_reachable = (classic->parent[i] != -1);
        int gb_reachable = (gb_parent[i] != -1);
        
        if (classic_reachable != gb_reachable) {
            consistent = 0;
            mismatches++;
            if (mismatches <= 5) {
                printf("  Mismatch at node %d: classic=%s, GraphBLAS=%s\n",
                       i, classic_reachable ? "reachable" : "unreachable",
                       gb_reachable ? "reachable" : "unreachable");
            }
        }
    }
    
    if (consistent) {
        printf("[VERIFICATION] ✓ Results are CONSISTENT between both algorithms\n");
    } else {
        printf("[VERIFICATION] ✗ Results INCONSISTENT! %d mismatches found\n", mismatches);
    }
    
    return consistent;
}

int main(int argc, char* argv[]) {
    print_banner();
    
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    const char* graph_file = argv[1];
    int source = (argc > 2) ? atoi(argv[2]) : 0;
    
    printf("\n[CONFIGURATION]\n");
    printf("  Graph file: %s\n", graph_file);
    printf("  Source node: %d\n", source);
    
    int* row_ptr = NULL;
    int* col_idx = NULL;
    double* values = NULL;
    int n = 0, nnz = 0;
    
    if (load_matrix_market(graph_file, &row_ptr, &col_idx, &values, &n, &nnz) != 0) {
        fprintf(stderr, "\n[ERROR] Failed to load graph file\n");
        return 1;
    }
    
    printf("CLASSIC BFS (Queue-based)\n");
    
    Timer timer;
    timer_start(&timer);
    BFSResult* classic_result = classic_bfs(n, row_ptr, col_idx, source);
    double classic_time = timer_elapsed_ms(&timer);
    printf("\n[TIMING] Classic BFS completed in: %.6f ms\n", classic_time);
    
    save_bfs_result("classic_bfs_result.txt", 
                    classic_result->parent, classic_result->level, 
                    n, classic_time, "Classic BFS (Queue)");
    
    printf("GRAPHBLAS BFS (Linear Algebra)\n");
    
    graphblas_init();
    GraphBLASGraph* gb_graph = graphblas_create_graph(n, row_ptr, col_idx, values, nnz);
    
    int* gb_level = NULL;
    double gb_time;
    int* gb_parent = graphblas_bfs(gb_graph, source, &gb_time, &gb_level);
    printf("\n[TIMING] GraphBLAS BFS completed in: %.6f ms\n", gb_time);
    
    save_bfs_result("graphblas_bfs_result.txt", 
                    gb_parent, gb_level, 
                    n, gb_time, "GraphBLAS BFS");
    

    printf("PERFORMANCE SUMMARY\n");
    printf("  Classic BFS:    %10.6f ms\n", classic_time);
    printf("  GraphBLAS BFS:  %10.6f ms\n", gb_time);
    
    if (classic_time < gb_time) {
        printf("  Classic BFS is %.2fx FASTER\n", gb_time / classic_time);
    } else if (gb_time < classic_time) {
        printf("  GraphBLAS BFS is %.2fx FASTER\n", classic_time / gb_time);
    } else {
        printf("  Both algorithms took the same time\n");
    }
    
    int consistent = verify_results(classic_result, gb_parent, gb_level, n);
    
    save_comparison(classic_time, gb_time, consistent, n);
    
    printf("\n[SUMMARY]\n");
    printf("  Graph size: %d nodes, %d edges\n", n, nnz);
    printf("  Reachable from source %d: %d/%d nodes (%.1f%%)\n",
           source, classic_result->visited_count, n,
           100.0 * classic_result->visited_count / n);
  
    int max_level = 0;
    for (int i = 0; i < n; i++) {
        if (classic_result->level[i] > max_level) {
            max_level = classic_result->level[i];
        }
    }
    printf("  Maximum BFS depth: %d\n", max_level);
    
    free_bfs_result(classic_result);
    free(gb_parent);
    free(gb_level);
    graphblas_free_graph(gb_graph);
    graphblas_finalize();
    free(row_ptr);
    free(col_idx);
    if (values) free(values);
    
    printf("\n[COMPLETE] Analysis finished successfully!\n");
    printf("Output files: classic_bfs_result.txt, graphblas_bfs_result.txt, bfs_comparison.txt\n\n");
    
    return 0;
}
