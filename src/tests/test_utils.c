#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"

Graph* create_test_graph() {
    Graph* g = create_graph(5);
    if (g == NULL) return NULL;
    
    add_edge(g, 0, 1);
    add_edge(g, 1, 2);
    add_edge(g, 0, 3);
    add_edge(g, 1, 4);
    add_edge(g, 3, 4);
    
    return g;
}

CSRMatrix* create_test_csr() {
    Graph* g = create_test_graph();
    if (g == NULL) return NULL;
    
    CSRMatrix* csr = graph_to_csr(g);
    delete_graph(g);
    return csr;
}

int check_all_visited(int* level, int n) {
    for (int i = 0; i < n; i++) {
        if (level[i] == -1) {
            return 0;  
        }
    }
    return 1; 
}

int compare_levels(int* level1, int* level2, int n) {
    for (int i = 0; i < n; i++) {
        if (level1[i] != level2[i]) {
            return 0;  
        }
    }
    return 1; 
}

int check_parent_correctness(int* parent, int n, int start_vertex) {
    // 1. Корень указывает на себя
    if (parent[start_vertex] != start_vertex) return 0;
    
    // 2. Все вершины посещены
    for (int i = 0; i < n; i++) {
        if (parent[i] == -1) return 0;
    }
    
    // 3. Нет петель у не-корневых
    for (int i = 0; i < n; i++) {
        if (i != start_vertex && parent[i] == i) {
            return 0;
        }
    }
    
    return 1;
}
