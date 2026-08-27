#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

Graph* create_graph(int n){
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    if (graph == NULL) {
        printf("Error: не удалось выделить память для графа\n");
        return NULL;
    }
    graph->num_of_vertices = n;
    graph->adjLists = (Node**)malloc(n * sizeof(Node*));
    if (graph->adjLists == NULL) {
        printf("Error: не удалось выделить память для списков смежности\n");
        free(graph);
        return NULL;
    }
    
    for (int i = 0; i < n; i++) {
        graph->adjLists[i] = NULL;
    }
    return graph;
};

void add_edge(Graph* g, int src, int dest) {
    if (g == NULL) {
        return;
    }
    
    if (src < 0 || src >= g->num_of_vertices || dest < 0 || dest >= g->num_of_vertices) {
        printf("Error: некорректные индексы вершин (%d, %d)\n", src, dest);
        return;
    }
    
    // src -> dest
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Ошибка: не удалось выделить память для ребра (%d, %d)\n", src, dest);
        return;
    }
    newNode->vertex = dest;
    newNode->next = g->adjLists[src];
    g->adjLists[src] = newNode;
    
    // dest -> src
    newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Error: не удалось выделить память для обратного ребра (%d, %d)\n", dest, src);
        return;
    }
    newNode->vertex = src;
    newNode->next = g->adjLists[dest];
    g->adjLists[dest] = newNode;
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
    
    int is_symmetric = 0;
    if (strstr(line, "symmetric") != NULL) {
        is_symmetric = 1;
    } else if (strstr(line, "general") != NULL) {
        is_symmetric = 0;
    } else {
        printf("Error: неподдерживаемый тип (только symmetric/general)\n");
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
        printf("Error: не удалось прочитать размеры\n");
        fclose(f);
        return NULL;
    }
    
    if (rows != cols) {
        printf("Error: матрица не квадратная (%d x %d)\n", rows, cols);
        fclose(f);
        return NULL;
    }
    
    Graph* graph = create_graph(rows);
    
    int src, dest;
    for (int i = 0; i < entries; i++) {
        if (fscanf(f, "%d %d", &src, &dest) != 2) {
            printf("Error: не удалось прочитать ребро %d\n", i);
            delete_graph(graph);
            fclose(f);
            return NULL;
        }
        src--;
        dest--;
        if (is_symmetric) {
            add_edge(graph, src, dest);
        } else {
            Node* newNode = (Node*)malloc(sizeof(Node));
            if (newNode == NULL) {
                printf("Error: не удалось выделить память для ребра (%d, %d)\n", src, dest);
                delete_graph(graph);
                fclose(f);
                return NULL;
            }
            newNode->vertex = dest;
            newNode->next = graph->adjLists[src];
            graph->adjLists[src] = newNode;
        }
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
        Node* current = g->adjLists[i];
        while (current != NULL) {
            Node* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(g->adjLists);
    free(g);
}

void delete_csr(CSRMatrix* csr) {
    if (csr == NULL) {
        return;
    }
    free(csr->row_ptr);
    free(csr->col_idx);
    free(csr);
}
