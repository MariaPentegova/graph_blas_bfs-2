#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "utils.h"
#include "classic_bfs.h"
#include "graphblas_bfs.h"

double get_time_in_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

double average(double* arr, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum / n;
}

double min_value(double* arr, int n) {
    double min = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] < min) min = arr[i];
    }
    return min;
}

double max_value(double* arr, int n) {
    double max = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > max) max = arr[i];
    }
    return max;
}

void run_benchmark(const char* graph_file) {
    printf("Benchmark: %s\n", graph_file);
    
    Graph* g = load_matrix(graph_file);
    if (g == NULL) {
        printf("Error: cannot load graph\n");
        return;
    }
    
    int start_vertex = find_max_degree_vertex(g);
    printf("Start vertex: %d\n", start_vertex);
    
    CSRMatrix* csr = graph_to_csr(g);
    if (csr == NULL) {
        delete_graph(g);
        return;
    }
    
    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    int step = csr->n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }
    
    printf("Sources: ");
    for (int i = 0; i < num_sources; i++) {
        printf("%d ", sources[i]);
    }
    printf("\n");
    
    int* parent = (int*)malloc(csr->n * sizeof(int));
    int* level = (int*)malloc(csr->n * sizeof(int));
    if (parent == NULL || level == NULL) {
        free(sources);
        free(parent);
        free(level);
        delete_graph(g);
        delete_csr(csr);
        return;
    }
    
    int num_runs = 10;
    double t_classic_parent[10];
    double t_classic_multisource[10];
    double t_graphblas_level[10];
    double t_graphblas_multisource[10];
    
    printf("\nRunning %d iterations...\n", num_runs);
    
    for (int run = 0; run < num_runs; run++) {
        double start, end;
        
        start = get_time_in_seconds();
        classic_parent_bfs(csr, start_vertex, parent);
        end = get_time_in_seconds();
        t_classic_parent[run] = (end - start) * 1e6;  
        
        start = get_time_in_seconds();
        classic_multisource_bfs(csr, sources, num_sources, parent);
        end = get_time_in_seconds();
        t_classic_multisource[run] = (end - start) * 1e6;
        

        start = get_time_in_seconds();
        graphblas_level_bfs(csr, start_vertex, level);
        end = get_time_in_seconds();
        t_graphblas_level[run] = (end - start) * 1e6;
        
        start = get_time_in_seconds();
        graphblas_multisource_level_bfs(csr, sources, num_sources, level);
        end = get_time_in_seconds();
        t_graphblas_multisource[run] = (end - start) * 1e6;
    }
    
    printf("\n Results (microseconds) \n");
    printf("Metric                    | Avg    | Min    | Max\n");
    printf("--------------------------|--------|--------|--------\n");
    printf("Classic Parent BFS        | %6.0f | %6.0f | %6.0f\n",
           average(t_classic_parent, num_runs),
           min_value(t_classic_parent, num_runs),
           max_value(t_classic_parent, num_runs));
    printf("Classic Multisource BFS   | %6.0f | %6.0f | %6.0f\n",
           average(t_classic_multisource, num_runs),
           min_value(t_classic_multisource, num_runs),
           max_value(t_classic_multisource, num_runs));
    printf("GraphBLAS Level BFS       | %6.0f | %6.0f | %6.0f\n",
           average(t_graphblas_level, num_runs),
           min_value(t_graphblas_level, num_runs),
           max_value(t_graphblas_level, num_runs));
    printf("GraphBLAS Multisource BFS | %6.0f | %6.0f | %6.0f\n",
           average(t_graphblas_multisource, num_runs),
           min_value(t_graphblas_multisource, num_runs),
           max_value(t_graphblas_multisource, num_runs));
    

    printf("\n Speedups \n");
    printf("GraphBLAS Level / Classic Parent: %.2fx\n",
           average(t_classic_parent, num_runs) / average(t_graphblas_level, num_runs));
    printf("GraphBLAS Multisource / Classic Multisource: %.2fx\n",
           average(t_classic_multisource, num_runs) / average(t_graphblas_multisource, num_runs));
    printf("Classic Multisource / Classic Parent: %.2fx\n",
           average(t_classic_parent, num_runs) / average(t_classic_multisource, num_runs));
    printf("GraphBLAS Multisource / GraphBLAS Level: %.2fx\n",
           average(t_graphblas_level, num_runs) / average(t_graphblas_multisource, num_runs));
    
    free(sources);
    free(parent);
    free(level);
    delete_graph(g);
    delete_csr(csr);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <graph.mtx>\n", argv[0]);
        printf("Example: %s ../graphs/g1.mtx\n", argv[0]); //потом название графа впишу
        return 1;
    }
    
    run_benchmark(argv[1]);
    
    return 0;
}
