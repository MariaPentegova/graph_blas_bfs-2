#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <GraphBLAS.h>
#include "utils.h"
#include "graphblas_bfs.h"

/* Состояние модуля: создаётся один раз в graphblas_init, переиспользуется
 * во всех вызовах BFS, освобождается один раз в graphblas_finalize. */
static bool g_initialized = false;
static GrB_Descriptor g_desc_visited_comp_replace = NULL; /* mask=visited, комплемент + replace */
static GrB_Monoid g_lor_monoid = NULL;
static GrB_Semiring g_lor_land_semiring = NULL;

int graphblas_init(void) {
    if (g_initialized) {
        return 0;
    }

    GrB_Info info = GrB_init(GrB_NONBLOCKING);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "GrB_init завершился с ошибкой (%d)\n", (int)info);
        return -1;
    }

#ifdef _OPENMP
    printf("GraphBLAS инициализирован с параллелизацией (%d потоков)\n", omp_get_max_threads());
#else
    printf("GraphBLAS инициализирован\n");
#endif

    /* Дескриптор: маска = visited, применяется в комплементе (т.е. "ещё
     * НЕ посещено") + REPLACE (старое содержимое выходного вектора
     * стирается, а не сохраняется вне маски). Это стандартный паттерн
     * для BFS на GraphBLAS и то, что даёт GraphBLAS переключаться между
     * push/pull в зависимости от плотности фронтира. */
    GrB_Descriptor_new(&g_desc_visited_comp_replace);
    GrB_Descriptor_set(g_desc_visited_comp_replace, GrB_OUTP, GrB_REPLACE);
    GrB_Descriptor_set(g_desc_visited_comp_replace, GrB_MASK, GrB_COMP);

    /* Встроенные GrB_LOR / GrB_LAND вместо пользовательских функций —
     * у них есть быстрые векторизованные реализации внутри
     * SuiteSparse:GraphBLAS, в отличие от произвольных function pointer'ов. */
    GrB_Monoid_new_BOOL(&g_lor_monoid, GrB_LOR, false);
    GrB_Semiring_new(&g_lor_land_semiring, g_lor_monoid, GrB_LAND);

    g_initialized = true;
    return 0;
}

void graphblas_finalize(void) {
    if (!g_initialized) {
        return;
    }
    GrB_Descriptor_free(&g_desc_visited_comp_replace);
    GrB_Semiring_free(&g_lor_land_semiring);
    GrB_Monoid_free(&g_lor_monoid);
    GrB_finalize();
    g_initialized = false;
}

int graphblas_build_matrix(CSRMatrix* csr, void** A_out) {
    if (csr == NULL || A_out == NULL) {
        return -1;
    }
    if (!g_initialized) {
        fprintf(stderr, "Error: graphblas_init() нужно вызвать до graphblas_build_matrix()\n");
        return -1;
    }

    GrB_Matrix* mat = (GrB_Matrix*)A_out;

    GrB_Index n = (GrB_Index)csr->n;
    GrB_Index total_edges = (GrB_Index)csr->row_ptr[csr->n];

    /* import() ЗАБИРАЕТ буферы Ap/Aj/Ax себе — после успешного вызова ими
     * распоряжается сама матрица, и освобождать их вручную больше нельзя.
     * Поэтому нельзя отдавать csr->row_ptr/csr->col_idx напрямую: это тот
     * же кусок памяти, которым продолжает владеть CSRMatrix и который
     * позже освобождает delete_csr() — вышло бы двойное освобождение.
     * Плюс это ещё и решает вопрос ширины типа: копия делается поэлементно
     * с явным приведением к GrB_Index, независимо от того, int* или уже
     * GrB_Index* использован в CSRMatrix. */
    GrB_Index* row_ptr = (GrB_Index*)malloc((n + 1) * sizeof(GrB_Index));
    GrB_Index* col_idx = (GrB_Index*)malloc(total_edges * sizeof(GrB_Index));
    bool* vals = (bool*)malloc(sizeof(bool));
    if (row_ptr == NULL || col_idx == NULL || vals == NULL) {
        free(row_ptr);
        free(col_idx);
        free(vals);
        fprintf(stderr, "Error: не удалось выделить память под импорт в GraphBLAS\n");
        return -1;
    }
    for (GrB_Index i = 0; i <= n; i++) {
        row_ptr[i] = (GrB_Index)csr->row_ptr[i];
    }
    for (GrB_Index i = 0; i < total_edges; i++) {
        col_idx[i] = (GrB_Index)csr->col_idx[i];
    }
    vals[0] = true;

    GrB_Index Ap_size = (n + 1) * sizeof(GrB_Index);
    GrB_Index Aj_size = total_edges * sizeof(GrB_Index);
    GrB_Index Ax_size = sizeof(bool);
    void* Ax = vals;

    /* Даже при iso=true нужен реальный указатель на ОДНО значение —
     * NULL/0 здесь были причиной сегфолта внутри GraphBLAS при проверке. */
    GrB_Info info = GxB_Matrix_import_CSR(
        mat, GrB_BOOL, n, n,
        &row_ptr, &col_idx, &Ax,
        Ap_size, Aj_size, Ax_size,
        true,   /* iso: все значения true, общее для всех элементов */
        false,  /* jumbled: индексы столбцов в каждой строке уже отсортированы */
        NULL
    );

    if (info != GrB_SUCCESS) {
        /* при ошибке буферы GraphBLAS не забрал — освобождаем сами */
        free(row_ptr);
        free(col_idx);
        free(vals);
        fprintf(stderr, "GxB_Matrix_import_CSR завершился с ошибкой (%d)\n", (int)info);
        return -1;
    }
    return 0;
}

void graphblas_free_matrix(void* A_handle) {
    if (A_handle == NULL) {
        return;
    }
    GrB_Matrix A = (GrB_Matrix)A_handle;
    GrB_Matrix_free(&A);
}

int graphblas_level_bfs(CSRMatrix* csr, void* A_handle, int start_vertex, int* level) {
    if (csr == NULL || level == NULL || A_handle == NULL ||
        start_vertex < 0 || start_vertex >= csr->n) {
        return -1;
    }
    if (!g_initialized) {
        fprintf(stderr, "Error: graphblas_init() нужно вызвать до graphblas_level_bfs()\n");
        return -1;
    }

    int n = csr->n;
    GrB_Matrix A = (GrB_Matrix)A_handle;

    GrB_Vector level_vec = NULL;
    GrB_Vector frontier = NULL;
    GrB_Vector visited = NULL;
    GrB_Vector new_frontier = NULL;

    GrB_Vector_new(&level_vec, GrB_INT32, n);
    GrB_Vector_new(&frontier, GrB_BOOL, n);
    GrB_Vector_new(&visited, GrB_BOOL, n);
    GrB_Vector_new(&new_frontier, GrB_BOOL, n);

    GrB_Vector_setElement_INT32(level_vec, 0, start_vertex);
    GrB_Vector_setElement_BOOL(frontier, true, start_vertex);
    GrB_Vector_setElement_BOOL(visited, true, start_vertex);

    for (int i = 0; i < n; i++) {
        level[i] = -1;
    }
    level[start_vertex] = 0;

    int current_level = 0;
    GrB_Index nvals = 1;

    while (nvals > 0) {
        /* new_frontier<!visited,replace> = frontier' * A (LOR-LAND) —
         * GrB_vxm, а не GrB_mxv: нам нужны ИСХОДЯЩИЕ соседи текущего
         * фронтира (frontier' * A), а не входящие (A * frontier). При
         * "row = вершина, col_idx = её исходящие соседи" (обычный CSR)
         * GrB_mxv обходил бы граф в обратную сторону — с ним BFS от
         * вершины без входящих рёбер сразу останавливался бы. */
        GrB_vxm(
            new_frontier,
            visited,
            NULL,
            g_lor_land_semiring,
            frontier,
            A,
            g_desc_visited_comp_replace
        );

        GrB_eWiseAdd(
            visited,
            NULL,
            NULL,
            GrB_LOR,
            visited,
            new_frontier,
            NULL
        );

        GrB_assign(
            level_vec,
            new_frontier,
            NULL,
            current_level + 1,
            GrB_ALL,
            n,
            NULL
        );

        /* Обмен хендлами вместо clear+copy: new_frontier на следующей
         * итерации всё равно будет полностью перезаписан (REPLACE). */
        GrB_Vector tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;

        GrB_Vector_nvals(&nvals, frontier);
        current_level++;
    }

    for (int i = 0; i < n; i++) {
        GrB_Vector_extractElement_INT32(&level[i], level_vec, i);
    }

    GrB_Vector_free(&level_vec);
    GrB_Vector_free(&frontier);
    GrB_Vector_free(&visited);
    GrB_Vector_free(&new_frontier);

    return 0;
}

int graphblas_multisource_level_bfs(CSRMatrix* csr, void* A_handle,
                                     int* sources, int num_sources, int* level) {
    if (csr == NULL || level == NULL || A_handle == NULL ||
        sources == NULL || num_sources <= 0) {
        return -1;
    }
    if (!g_initialized) {
        fprintf(stderr, "Error: graphblas_init() нужно вызвать до graphblas_multisource_level_bfs()\n");
        return -1;
    }

    int n = csr->n;
    GrB_Matrix A = (GrB_Matrix)A_handle;

    GrB_Vector level_vec = NULL;
    GrB_Vector frontier = NULL;
    GrB_Vector visited = NULL;
    GrB_Vector new_frontier = NULL;

    GrB_Vector_new(&level_vec, GrB_INT32, n);
    GrB_Vector_new(&frontier, GrB_BOOL, n);
    GrB_Vector_new(&visited, GrB_BOOL, n);
    GrB_Vector_new(&new_frontier, GrB_BOOL, n);

    for (int i = 0; i < n; i++) {
        level[i] = -1;
    }

    for (int i = 0; i < num_sources; i++) {
        int s = sources[i];
        if (s >= 0 && s < n) {
            GrB_Vector_setElement_INT32(level_vec, 0, s);
            GrB_Vector_setElement_BOOL(frontier, true, s);
            GrB_Vector_setElement_BOOL(visited, true, s);
            level[s] = 0;
        }
    }

    int current_level = 0;
    GrB_Index nvals = (GrB_Index)num_sources;

    while (nvals > 0) {
        GrB_vxm(
            new_frontier,
            visited,
            NULL,
            g_lor_land_semiring,
            frontier,
            A,
            g_desc_visited_comp_replace
        );

        GrB_eWiseAdd(
            visited,
            NULL,
            NULL,
            GrB_LOR,
            visited,
            new_frontier,
            NULL
        );

        GrB_assign(
            level_vec,
            new_frontier,
            NULL,
            current_level + 1,
            GrB_ALL,
            n,
            NULL
        );

        GrB_Vector tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;

        GrB_Vector_nvals(&nvals, frontier);
        current_level++;
    }

    for (int i = 0; i < n; i++) {
        GrB_Vector_extractElement_INT32(&level[i], level_vec, i);
    }

    GrB_Vector_free(&level_vec);
    GrB_Vector_free(&frontier);
    GrB_Vector_free(&visited);
    GrB_Vector_free(&new_frontier);

    return 0;
}
