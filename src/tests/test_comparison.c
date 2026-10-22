#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "test_utils.h"
#include "../main/classic_bfs.h"
#include "../main/graphblas_bfs.h"

static void check_parent_tree(CSRMatrix* csr, int* parent, int start_vertex) {
    int n = csr->n;
    assert(parent[start_vertex] == start_vertex);
    for (int v = 0; v < n; v++) {
        if (v == start_vertex) continue;
        if (parent[v] == -1) continue;
        assert(parent[v] != v);
        int found = 0;
        int begin = csr->row_ptr[v];
        int end = csr->row_ptr[v + 1];
        for (int j = begin; j < end; j++) {
            if (csr->col_idx[j] == parent[v]) { found = 1; break; }
        }
        assert(found == 1);
    }
}

void compare_single_source_classic_vs_graphblas() {
    printf("Test Single-Source BFS (Classic vs GraphBLAS)\n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    int n = csr->n;
    int* parent_classic = (int*)malloc(n * sizeof(int));
    int* parent_graphblas = (int*)malloc(n * sizeof(int));
    assert(parent_classic != NULL);
    assert(parent_graphblas != NULL);

    Graph* temp_g = create_test_graph();
    int start_vertex = find_max_degree_vertex(temp_g);
    delete_graph(temp_g);
    printf("Start vertex (max degree): %d\n", start_vertex);

    csr_parent_bfs(csr, start_vertex, parent_classic);
    printf("classic done\n"); fflush(stdout);

    void* A = NULL;
    int rc = graphblas_build_matrix(csr, &A);
    assert(rc == 0);
    (void)rc;
    printf("build done\n"); fflush(stdout);

    rc = graphblas_parent_bfs(csr, A, start_vertex, parent_graphblas);
    assert(rc == 0);
    (void)rc;
    printf("graphblas parent done\n"); fflush(stdout);

    graphblas_free_matrix(A);
    printf("free done\n"); fflush(stdout);

    printf("parent_classic: ");
    for (int i = 0; i < n; i++) printf("%d ", parent_classic[i]);
    printf("\nparent_graphblas: ");
    for (int i = 0; i < n; i++) printf("%d ", parent_graphblas[i]);
    printf("\n"); fflush(stdout);

    check_parent_tree(csr, parent_classic, start_vertex);
    check_parent_tree(csr, parent_graphblas, start_vertex);

    for (int i = 0; i < n; i++) {
        assert((parent_classic[i] == -1) == (parent_graphblas[i] == -1));
    }

    int* level_classic = (int*)malloc(n * sizeof(int));
    int* level_graphblas = (int*)malloc(n * sizeof(int));
    assert(level_classic != NULL && level_graphblas != NULL);
    for (int i = 0; i < n; i++) { level_classic[i] = -1; level_graphblas[i] = -1; }
    for (int i = 0; i < n; i++) {
        if (parent_classic[i] == -1) { level_classic[i] = -1; continue; }
        int depth = 0, v = i;
        while (parent_classic[v] != v) { v = parent_classic[v]; depth++; }
        level_classic[i] = depth;
    }
    for (int i = 0; i < n; i++) {
        if (parent_graphblas[i] == -1) { level_graphblas[i] = -1; continue; }
        int depth = 0, v = i;
        while (parent_graphblas[v] != v) { v = parent_graphblas[v]; depth++; }
        level_graphblas[i] = depth;
    }
    for (int i = 0; i < n; i++) {
        assert(level_classic[i] == level_graphblas[i]);
    }

    free(parent_classic);
    free(parent_graphblas);
    free(level_classic);
    free(level_graphblas);
    delete_csr(csr);
    printf("Test Single-Source compare passed!\n\n");
}

void compare_multisource_classic_vs_graphblas() {
    printf("Test Multisource BFS (Classic vs GraphBLAS)\n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    int n = csr->n;
    int* parent_classic = (int*)malloc(n * sizeof(int));
    int* parent_graphblas = (int*)malloc(n * sizeof(int));
    assert(parent_classic != NULL && parent_graphblas != NULL);

    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    assert(sources != NULL);

    int step = n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }

    printf("Sources: ");
    for (int i = 0; i < num_sources; i++) printf("%d ", sources[i]);
    printf("\n");

    csr_multisource_bfs(csr, sources, num_sources, parent_classic);

    void* A = NULL;
    int rc = graphblas_build_matrix(csr, &A);
    assert(rc == 0);
    (void)rc;

    rc = graphblas_multisource_parent_bfs(csr, A, sources, num_sources, parent_graphblas);
    assert(rc == 0);
    (void)rc;
    graphblas_free_matrix(A);

    for (int i = 0; i < n; i++) {
        assert((parent_classic[i] == -1) == (parent_graphblas[i] == -1));
    }

    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        assert(parent_classic[s] == s);
        assert(parent_graphblas[s] == s);
    }

    int* level_classic = (int*)malloc(n * sizeof(int));
    int* level_graphblas = (int*)malloc(n * sizeof(int));
    assert(level_classic != NULL && level_graphblas != NULL);
    for (int i = 0; i < n; i++) { level_classic[i] = -1; level_graphblas[i] = -1; }
    for (int i = 0; i < n; i++) {
        if (parent_classic[i] == -1) continue;
        int depth = 0, v = i;
        while (parent_classic[v] != v) { v = parent_classic[v]; depth++; }
        level_classic[i] = depth;
    }
    for (int i = 0; i < n; i++) {
        if (parent_graphblas[i] == -1) continue;
        int depth = 0, v = i;
        while (parent_graphblas[v] != v) { v = parent_graphblas[v]; depth++; }
        level_graphblas[i] = depth;
    }
    for (int i = 0; i < n; i++) {
        assert(level_classic[i] == level_graphblas[i]);
    }

    free(parent_classic);
    free(parent_graphblas);
    free(level_classic);
    free(level_graphblas);
    free(sources);
    delete_csr(csr);
    printf("Test Multisource compare passed!\n\n");
}

int main() {
    if (graphblas_init() != 0) {
        return 1;
    }

    compare_single_source_classic_vs_graphblas();
    compare_multisource_classic_vs_graphblas();

    graphblas_finalize();
    return 0;
}
