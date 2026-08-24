#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"         
#include "classic_bfs.h"   
#include "graphblas_bfs.h" 

int main() {
    Graph* g = load_matrix("graph.mtx");
    if (g == NULL) {
        return 1;
    }
    
    CSRMatrix* csr = graph_to_csr(g);
    if (csr == NULL) {
        delete_graph(g);
        return 1;
    }
    
    int start_vertex = find_max_degree_vertex(g);
    printf("Стартовая вершина для parent BFS: %d\n", start_vertex);
    
    int num_sources = 4;  // или 8, 16 — сколько хотите
    int* sources = (int*)malloc(num_sources * sizeof(int));
    if (sources == NULL) {
        printf("Error: не удалось выделить память для источников\n");
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }
    
    int step = csr->n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }
    
    printf("Источники для multisource BFS: ");
    for (int i = 0; i < num_sources; i++) {
        printf("%d ", sources[i]);
    }
    printf("\n");
    
    int* parent = (int*)malloc(csr->n * sizeof(int));
    if (parent == NULL) {
        printf("Error: не удалось выделить память для parent\n");
        free(sources);
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }
    
    printf("\n Parent BFS \n");
    csr_parent_bfs(csr, start_vertex, parent);
    // можно вывести результат или замерить время
    
    printf("\n Multisource BFS n");
    csr_multisource_bfs(csr, sources, num_sources, parent);
    // можно вывести результат или замерить время

    csr_to_graphblas(CSRMatrix* csr, GrB_Matrix* A);
    int graphblas_level_bfs(CSRMatrix* csr, int start_vertex, int* level);
    //получается, здесь замер времени
    int graphblas_multisource_level_bfs(CSRMatrix* csr, int* sources, int num_sources, int* level);
    //получается, здесь замер времени
    free(parent);
    free(sources);
    delete_graph(g);
    delete_csr(csr);
    
    return 0;
}
