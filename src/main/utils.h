#ifndef UTILS_H
#define UTILS_H

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    struct timeval start;
    struct timeval end;
} Timer;

void timer_start(Timer* timer);
double timer_elapsed_ms(Timer* timer);
double timer_elapsed_us(Timer* timer);
double timer_elapsed_seconds(Timer* timer);

int load_matrix_market(const char* filename, 
                        int** row_ptr, int** col_idx, double** values,
                        int* n, int* nnz);

void save_bfs_result(const char* filename, int* parent, int* level, int n, double time_ms, const char* algorithm);

#endif