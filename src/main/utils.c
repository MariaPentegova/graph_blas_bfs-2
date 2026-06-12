#include "utils.h"
#include <string.h>
#include <ctype.h>

void timer_start(Timer* timer) {
    gettimeofday(&timer->start, NULL);
}

double timer_elapsed_ns(Timer* timer) {
    gettimeofday(&timer->end, NULL);
    double elapsed = (timer->end.tv_sec - timer->start.tv_sec) * 1000000000.0;
    elapsed += (timer->end.tv_usec - timer->start.tv_usec) * 1000.0;
    return elapsed;
}

double timer_elapsed_ms(Timer* timer) {
    gettimeofday(&timer->end, NULL);
    double elapsed = (timer->end.tv_sec - timer->start.tv_sec) * 1000.0;
    elapsed += (timer->end.tv_usec - timer->start.tv_usec) / 1000.0;
    return elapsed;
}

double timer_elapsed_us(Timer* timer) {
    gettimeofday(&timer->end, NULL);
    double elapsed = (timer->end.tv_sec - timer->start.tv_sec) * 1000000.0;
    elapsed += (timer->end.tv_usec - timer->start.tv_usec);
    return elapsed;
}

double timer_elapsed_seconds(Timer* timer) {
    gettimeofday(&timer->end, NULL);
    double elapsed = (timer->end.tv_sec - timer->start.tv_sec);
    elapsed += (timer->end.tv_usec - timer->start.tv_usec) / 1000000.0;
    return elapsed;
}

int load_matrix_market(const char* filename, 
                        int** row_ptr, int** col_idx, double** values,
                        int* n, int* nnz) {
    FILE* f = fopen(filename, "r");
    if (!f) return -1;
    
    char line[1024];
    int rows = 0, cols = 0, nonzeros = 0;
    int header_read = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '%') continue;
        if (!header_read) {
            if (sscanf(line, "%d %d %d", &rows, &cols, &nonzeros) == 3) {
                *n = rows;
                *nnz = nonzeros;
                header_read = 1;
                break;
            }
        }
    }
    
    if (!header_read) {
        fclose(f);
        return -2;
    }
    
    int* temp_row = (int*)malloc(nonzeros * sizeof(int));
    int* temp_col = (int*)malloc(nonzeros * sizeof(int));
    
    rewind(f);
    int idx = 0;
    int in_data = 0;
    
    while (fgets(line, sizeof(line), f) && idx < nonzeros) {
        if (line[0] == '%') continue;
        if (strlen(line) <= 1) continue;
        
        if (!in_data) {
            int r, c;
            if (sscanf(line, "%d %d", &r, &c) == 2) {
                in_data = 1;
            }
        }
        
        if (in_data) {
            int r, c;
            if (sscanf(line, "%d %d", &r, &c) == 2) {
                temp_row[idx] = r - 1;
                temp_col[idx] = c - 1;
                idx++;
            }
        }
    }
    fclose(f);
    
    *row_ptr = (int*)calloc(rows + 1, sizeof(int));
    for (int i = 0; i < nonzeros; i++) {
        (*row_ptr)[temp_row[i] + 1]++;
    }
    for (int i = 1; i <= rows; i++) {
        (*row_ptr)[i] += (*row_ptr)[i - 1];
    }
    
    *col_idx = (int*)malloc(nonzeros * sizeof(int));
    *values = NULL;
    
    int* pos = (int*)malloc(rows * sizeof(int));
    for (int i = 0; i < rows; i++) pos[i] = (*row_ptr)[i];
    
    for (int i = 0; i < nonzeros; i++) {
        int r = temp_row[i];
        (*col_idx)[pos[r]++] = temp_col[i];
    }
    
    free(pos);
    free(temp_row);
    free(temp_col);
    
    return 0;
}

void save_bfs_result(const char* filename, int* parent, int* level, int n, double time_ms, const char* algorithm) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "# %s\n# Time: %.6f ms\n# Node Parent Level\n", algorithm, time_ms);
    for (int i = 0; i < n; i++) {
        fprintf(f, "%d %d %d\n", i, parent[i], level[i]);
    }
    fclose(f);
}