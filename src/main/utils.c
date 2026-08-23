#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"


//что-то не так с bfs_func, как компилятор поймёт по жтому, какиефункции ему замерять??
double get_time_in_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

double measure_bfs_time(void (*bfs_func)(Graph*, int, int*), 
                         Graph* g, 
                         int start_vertex) {
    int* parent = (int*)malloc(g->num_of_vertices * sizeof(int));
    if (parent == NULL) {
        printf("Ошибка: не удалось выделить память\n");
        return -1.0;
    }
    
    double start = get_time_in_seconds();
    bfs_func(g, start_vertex, parent);
    double end = get_time_in_seconds();
    
    free(parent);
    return end - start;
}

double measure_bfs_time_graphblas(void (*bfs_func)(CSRMatrix*, int, int*), 
                                   CSRMatrix* csr, 
                                   int start_vertex) {
    int* parent = (int*)malloc(csr->n * sizeof(int));
    if (parent == NULL) {
        printf("Ошибка: не удалось выделить память\n");
        return -1.0;
    }
    
    double start = get_time_in_seconds();
    bfs_func(csr, start_vertex, parent);
    double end = get_time_in_seconds();
    
    free(parent);
    return end - start;
}

Graph* load_matrix(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        printf("Error: не удалось открыть файл %s\n", filename);
        return NULL;
    }
    
    char line[256];
    
    if (fgets(line, sizeof(line), f) == NULL) {
        printf("Error: пустой файл\n");
        fclose(f);
        return NULL;
    }
    
    if (strstr(line, "%%MatrixMarket") == NULL) {
        printf("Error: неверный формат (не .mtx)\n");
        fclose(f);
        return NULL;
    }

    if (strstr(line, "coordinate") == NULL) {
        printf("Error: поддерживается только coordinate\n");
        fclose(f);
        return NULL;
    }
    
    if (strstr(line, "symmetric") == NULL) {
        printf("Ошибка: поддерживается только symmetric (неориентированный)\n");
        fclose(f);
        return NULL;
    }
    
    if (strstr(line, "pattern") == NULL) {
        printf("Error: поддерживается только pattern (без весов)\n");
        fclose(f);
        return NULL;
    }
    
    while (fgets(line, sizeof(line), f) != NULL) {
        if (line[0] != '%') {
            break;
        }
    }
    
    int rows, cols, entries;
    if (sscanf(line, "%d %d %d", &rows, &cols, &entries) != 3) {
        printf("Ошибка: не удалось прочитать размеры\n");
        fclose(f);
        return NULL;
    }
    
    if (rows != cols) {
        printf("Ошибка: матрица не квадратная (%d x %d)\n", rows, cols);
        fclose(f);
        return NULL;
    }
    
    Graph* graph = create_graph(rows);
    if (graph == NULL) {
        printf("Ошибка: не удалось создать граф\n");
        fclose(f);
        return NULL;
    }
    
    int src, dest;
    for (int i = 0; i < entries; i++) {
        if (fscanf(f, "%d %d", &src, &dest) != 2) {
            printf("Ошибка: не удалось прочитать ребро %d\n", i);
            delete_graph(graph);
            fclose(f);
            return NULL;
        }
        src--;
        dest--;
        add_edge(graph, src, dest);
    }
    
    fclose(f);
    printf("Граф загружен: %d вершин, %d ребер\n", rows, entries);
    return graph;
}

CSRMatrix* graph_to_csr(Graph* graph) {
    if (graph == NULL) {
        return NULL;
    }
    
    CSRMatrix* csr = (CSRMatrix*)malloc(sizeof(CSRMatrix));
    csr->n = graph->num_of_vertices;
    
    csr->row_ptr = (int*)malloc((graph->num_of_vertices + 1) * sizeof(int));
    
    int total_edges = 0;
    csr->row_ptr[0] = 0;
    for (int i = 0; i < graph->num_of_vertices; i++) {
        int degree = 0;
        Node* current = graph->adjLists[i];
        while (current != NULL) {
            degree++;
            current = current->next;
        }
        total_edges += degree;
        csr->row_ptr[i + 1] = csr->row_ptr[i] + degree;
    }
    
    csr->col_idx = (int*)malloc(total_edges * sizeof(int));
    
    int* current_pos = (int*)calloc(graph->num_of_vertices, sizeof(int));
    for (int i = 0; i < graph->num_of_vertices; i++) {
        Node* current = graph->adjLists[i];
        while (current != NULL) {
            int pos = csr->row_ptr[i] + current_pos[i];
            csr->col_idx[pos] = current->vertex;
            current_pos[i]++;
            current = current->next;
        }
    }
    
    free(current_pos);
    return csr;
}

int find_max_degree_vertex(Graph* graph) {
    if (graph == NULL) {
        return -1;
    }
    
    int max_degree = -1;
    int max_vertex = 0;
    
    for (int i = 0; i < graph->num_of_vertices; i++) {
        int degree = 0;
        Node* current = graph->adjLists[i];
        while (current != NULL) {
            degree++;
            current = current->next;
        }
        if (degree > max_degree) {
            max_degree = degree;
            max_vertex = i;
        }
    }
    
    return max_vertex;
}

void delete_graph(Graph* g) {
    if (g == NULL) return;
    for (int i = 0; i < g->num_of_vertices; i++) {
        struct Node* current = g->adjLists[i];
        while (current != NULL) {
            struct Node* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(g->adjLists);
    free(g);
}

/* void save_bfs_result(const char* filename, int* parent, int* level, int n, double time_ms, const char* algorithm) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "# %s\n# Time: %.6f ms\n# Node Parent Level\n", algorithm, time_ms);
    for (int i = 0; i < n; i++) {
        fprintf(f, "%d %d %d\n", i, parent[i], level[i]);
    }
    fclose(f);
} */
