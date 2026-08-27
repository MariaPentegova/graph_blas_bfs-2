#ifndef GRAPHBLAS_BFS_H
#define GRAPHBLAS_BFS_H

#include "utils.h"   /* CSRMatrix */

/* Инициализация GraphBLAS (GrB_init) и создание переиспользуемых
 * объектов (дескриптор маски, семиринг LOR-LAND). Вызвать РОВНО ОДИН
 * РАЗ за всю программу, до первого замера времени и до первого
 * обращения к graphblas_build_matrix/graphblas_level_bfs. */
int graphblas_init(void);

/* Освобождает объекты, созданные в graphblas_init, и завершает
 * GraphBLAS (GrB_finalize). Вызвать один раз в самом конце программы,
 * после того как все GrB_Matrix-хендлы уже освобождены. */
void graphblas_finalize(void);

/* Строит GrB_Matrix из CSR ОДИН РАЗ. Хендл матрицы возвращается через
 * *A_out как void*, чтобы main.c не нужно было подключать GraphBLAS.h.
 * Построенную матрицу можно и нужно переиспользовать во всех
 * последующих вызовах BFS — не пересобирать её каждый раз. */
int graphblas_build_matrix(CSRMatrix* csr, void** A_out);

/* Освобождает матрицу, построенную graphblas_build_matrix. */
void graphblas_free_matrix(void* A_handle);

/* BFS с уровнями от одного источника. A_handle — то, что вернул
 * graphblas_build_matrix (матрица НЕ пересобирается внутри). */
int graphblas_level_bfs(CSRMatrix* csr, void* A_handle, int start_vertex, int* level);

/* Мультиисточниковый BFS с уровнями. */
int graphblas_multisource_level_bfs(CSRMatrix* csr, void* A_handle,
                                     int* sources, int num_sources, int* level);

#endif /* GRAPHBLAS_BFS_H */
