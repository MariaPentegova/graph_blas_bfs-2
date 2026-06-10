#ifndef UTILS_H
#define UTILS_H

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    struct timeval start;
    struct timeval end;
} Timer;

// Timer functions
void timer_start(Timer* timer);
double timer_elapsed_ms(Timer* timer);
double timer_elapsed_seconds(Timer* timer);

// Matrix Market loader
int load_matrix_market(const char* filename, 
                        int** row_ptr, int** col_idx, double** values,
                        int* n, int* nnz);

// Save BFS results
void save_bfs_result(const char* filename, int* parent, int* level, int n, double time_ms, const char* algorithm);

// Save comparison
void save_comparison(double classic_time, double graphblas_time, int consistent, int n);

#endif
