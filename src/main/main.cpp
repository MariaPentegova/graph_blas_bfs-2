#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <vector>
#include <utility>

extern "C" {
    #include "utils.h"
    #include "classic_bfs.h"
    #include "graphblas_bfs.h"
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Error: you didn't provide a file for analysis.\n");
        return 1;
    }
    const char* filename = argv[1];
    Graph* g = load_matrix(filename);
    if (g == NULL) {
        printf("Error: failed to load the graph\n");
        return 1;
    }

    int start_vertex = find_max_degree_vertex(g);
    printf("Vertex with the maximum degree: %d\n", start_vertex);

    CSRMatrix* csr = graph_to_csr(g);
    if (csr == NULL) {
        printf("Error: failed to convert to CSR\n");
        delete_graph(g);
        return 1;
    }

    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    if (sources == NULL) {
        printf("Error: failed to allocate memory for sources\n");
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }

    int step = csr->n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }

    printf("Sources for multisource BFS: ");
    for (int i = 0; i < num_sources; i++) {
        printf("%d ", sources[i]);
    }
    printf("\n");

    int* parent = (int*)malloc(csr->n * sizeof(int));
    int* level = (int*)malloc(csr->n * sizeof(int));
    if (parent == NULL || level == NULL) {
        printf("Error: failed to allocate memory\n");
        free(sources);
        free(parent);
        free(level);
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }

    struct timespec start, end;
    double elapsed;

    if (graphblas_init() != 0) {
        printf("Error: failed to initialize GraphBLAS\n");
        free(sources);
        free(parent);
        free(level);
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }

    void* gb_matrix = NULL;
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (graphblas_build_matrix(csr, &gb_matrix) != 0) {
        printf("Error: failed to construct GraphBLAS matrix\n");
        graphblas_finalize();
        free(sources);
        free(parent);
        free(level);
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("\n GraphBLAS: matrix construction (once, not part of BFS) \n");
    printf("Time: %.6f сек\n", elapsed);

    printf("\n Classic Parent BFS \n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    csr_parent_bfs(csr, start_vertex, parent);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time: %.6f сек\n", elapsed);

    printf("\n Boost BGL Parent BFS \n");

    for (int i = 0; i < csr->n; i++) parent[i] = -1;
    parent[start_vertex] = start_vertex;

    clock_gettime(CLOCK_MONOTONIC, &start);

    int total_edges = csr->row_ptr[csr->n];
    std::vector<std::pair<int, int>> edge_list;
    edge_list.reserve(total_edges);

    for (int u = 0; u < csr->n; ++u) {
        int row_start = csr->row_ptr[u];
        int row_end = csr->row_ptr[u + 1];
        for (int i = row_start; i < row_end; ++i) {
            int v = csr->col_idx[i];
            edge_list.push_back(std::make_pair(u, v));
        }
    }

    typedef boost::compressed_sparse_row_graph<boost::directedS, boost::no_property, boost::no_property, boost::no_property, int, int> BGLGraph;
    BGLGraph bgl_g(boost::edges_are_sorted, edge_list.begin(), edge_list.end(), csr->n);

    boost::breadth_first_search(bgl_g, start_vertex,
                                boost::visitor(boost::make_bfs_visitor(
                                    boost::record_predecessors(parent, boost::on_tree_edge())
                                ))
    );

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time: %.6f сек\n", elapsed);

    printf("\n Classic Multisource BFS \n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    csr_multisource_bfs(csr, sources, num_sources, parent);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time: %.6f сек\n", elapsed);

    printf("\n GraphBLAS Level BFS \n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    graphblas_level_bfs(csr, gb_matrix, start_vertex, level);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time: %.6f сек\n", elapsed);

    printf("\n GraphBLAS Multisource Level BFS \n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    graphblas_multisource_level_bfs(csr, gb_matrix, sources, num_sources, level);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time: %.6f сек\n", elapsed);

    graphblas_free_matrix(gb_matrix);
    graphblas_finalize();

    free(sources);
    free(parent);
    free(level);
    delete_graph(g);
    delete_csr(csr);

    return 0;
}

