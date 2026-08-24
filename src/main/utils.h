#ifndef UTILS_H
#define UTILS_H

struct Node {
    int vertex;
    struct Node* next;
};

struct Graph {
    int num_of_vertices;
    struct Node** adjLists;
};

struct CSRMatrix {
    int n;
    int* row_ptr;
    int* col_idx;
};

typedef struct Node Node;
typedef struct Graph Graph;
typedef struct CSRMatrix CSRMatrix;

Graph* create_graph(int n);
void add_edge(struct Graph* g, int src, int dest);
Graph* load_matrix(const char* filename);
CSRMatrix* graph_to_csr(Graph* g);
int find_max_degree_vertex(Graph* g);
void delete_graph(Graph* g);

#endif
