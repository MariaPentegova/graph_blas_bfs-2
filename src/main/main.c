#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "utils.h"
#include "classic_bfs.h"
#include "graphblas_bfs.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Error: не написан файл для анализа");
        return 1;
    }
    const char* filename = argv[1];
    Graph* g = load_matrix(filename);
    if (g == NULL) {
        printf("Error: не удалось загрузить граф\n");
        return 1;
    }

    int start_vertex = find_max_degree_vertex(g);
    printf("Вершина с максимальной степенью: %d\n", start_vertex);

    CSRMatrix* csr = graph_to_csr(g);
    if (csr == NULL) {
        printf("Errror: не удалось преобразовать в CSR\n");
        delete_graph(g);
        return 1;
    }

    int num_sources = 4;
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
    int* level = (int*)malloc(csr->n * sizeof(int));
    if (parent == NULL || level == NULL) {
        printf("Error: не удалось выделить память\n");
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
        printf("Error: не удалось инициализировать GraphBLAS\n");
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
        printf("Error: не удалось построить GraphBLAS-матрицу\n");
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
    printf("\n GraphBLAS: построение матрицы (один раз, не входит в BFS) \n");
    printf("Время: %.6f сек\n", elapsed);

    printf("\n Classic Parent BFS \n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    csr_parent_bfs(csr, start_vertex, parent);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Время: %.6f сек\n", elapsed);

    printf("\n Classic Multisource BFS \n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    csr_multisource_bfs(csr, sources, num_sources, parent);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Время: %.6f сек\n", elapsed);

    printf("\n GraphBLAS Level BFS \n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    graphblas_level_bfs(csr, gb_matrix, start_vertex, level);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Время: %.6f сек\n", elapsed);

    printf("\n GraphBLAS Multisource Level BFS \n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    graphblas_multisource_level_bfs(csr, gb_matrix, sources, num_sources, level);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Время: %.6f сек\n", elapsed);

    graphblas_free_matrix(gb_matrix);
    graphblas_finalize();

    free(sources);
    free(parent);
    free(level);
    delete_graph(g);
    delete_csr(csr);

    return 0;
}

