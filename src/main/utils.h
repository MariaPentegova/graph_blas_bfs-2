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

struct Graph* create_graph(int n);
void add_edge(struct Graph* g, int src, int dest);
struct Graph* load_matrix(const char* filename);
struct CSRMatrix* graph_to_csr(struct Graph* g);
int find_max_degree_vertex(struct Graph* g);
void delete_graph(struct Graph* g);

#endif
