#include "utils.h"
#include <string.h>
#include <ctype.h>

void timer_start(Timer* timer) {
    gettimeofday(&timer->start, NULL);
}

double timer_elapsed_ms(Timer* timer) {
    gettimeofday(&timer->end, NULL);
    double elapsed = (timer->end.tv_sec - timer->start.tv_sec) * 1000.0;
    elapsed += (timer->end.tv_usec - timer->start.tv_usec) / 1000.0;
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
    if (!f) {
        printf("Error: Cannot open file %s\n", filename);
        return -1;
    }
    
    char line[1024];
    int rows = 0, cols = 0, nonzeros = 0;
    int header_read = 0;
    char symmetry[20] = "general";
    int is_pattern = 0;
    int line_num = 0;
    
    printf("\n[LOADER] Reading Matrix Market file: %s\n", filename);
    
    // Read header
    while (fgets(line, sizeof(line), f)) {
        line_num++;
        line[strcspn(line, "\n")] = 0;
        
        if (line[0] == '%') {
            if (strstr(line, "%%MatrixMarket")) {
                char pattern[20], field[20];
                if (sscanf(line, "%%%%MatrixMarket %s %s %s %s", pattern, pattern, field, symmetry) >= 3) {
                    if (strcmp(field, "pattern") == 0) {
                        is_pattern = 1;
                        printf("[LOADER] Format: pattern (structural only)\n");
                    } else if (strcmp(field, "real") == 0) {
                        printf("[LOADER] Format: real values\n");
                    }
                    printf("[LOADER] Symmetry: %s\n", symmetry);
                }
            }
            continue;
        }
        
        if (!header_read) {
            if (sscanf(line, "%d %d %d", &rows, &cols, &nonzeros) == 3) {
                *n = rows;
                *nnz = nonzeros;
                header_read = 1;
                printf("[LOADER] Dimensions: %d x %d, stored entries: %d\n", rows, cols, nonzeros);
                break;
            }
        }
    }
    
    if (!header_read) {
        fclose(f);
        return -2;
    }
    
    // Allocate temporary storage
    int* temp_row = (int*)malloc(nonzeros * sizeof(int));
    int* temp_col = (int*)malloc(nonzeros * sizeof(int));
    double* temp_val = is_pattern ? NULL : (double*)malloc(nonzeros * sizeof(double));
    
    if (!temp_row || !temp_col) {
        printf("[LOADER] Memory allocation failed\n");
        fclose(f);
        return -3;
    }
    
    // Read data
    int current_nz = 0;
    rewind(f);
    int in_data = 0;
    
    while (fgets(line, sizeof(line), f) && current_nz < nonzeros) {
        if (line[0] == '%') continue;
        if (strlen(line) <= 1) continue;
        
        if (!in_data && (line[0] != '%')) {
            int r, c;
            if (sscanf(line, "%d %d", &r, &c) == 2) {
                in_data = 1;
            }
        }
        
        if (in_data) {
            int r, c;
            double v = 1.0;
            
            if (is_pattern) {
                if (sscanf(line, "%d %d", &r, &c) == 2) {
                    temp_row[current_nz] = r - 1;  // to 0-based
                    temp_col[current_nz] = c - 1;
                    current_nz++;
                }
            } else {
                if (sscanf(line, "%d %d %lf", &r, &c, &v) == 3) {
                    temp_row[current_nz] = r - 1;
                    temp_col[current_nz] = c - 1;
                    temp_val[current_nz] = v;
                    current_nz++;
                }
            }
        }
    }
    fclose(f);
    
    // Handle symmetric matrices (store both directions)
    int is_symmetric = (strcmp(symmetry, "symmetric") == 0);
    int total_nnz = nonzeros;
    
    if (is_symmetric) {
        total_nnz = 0;
        for (int i = 0; i < nonzeros; i++) {
            total_nnz++;
            if (temp_row[i] != temp_col[i]) {
                total_nnz++;
            }
        }
        printf("[LOADER] Symmetric expansion: %d total edges\n", total_nnz);
    }
    
    // Build CSR format
    *row_ptr = (int*)calloc(rows + 1, sizeof(int));
    
    // Count degrees
    for (int i = 0; i < nonzeros; i++) {
        int r = temp_row[i];
        (*row_ptr)[r + 1]++;
        if (is_symmetric && temp_row[i] != temp_col[i]) {
            (*row_ptr)[temp_col[i] + 1]++;
        }
    }
    
    // Prefix sum
    for (int i = 1; i <= rows; i++) {
        (*row_ptr)[i] += (*row_ptr)[i - 1];
    }
    
    // Allocate column index array
    *col_idx = (int*)malloc(total_nnz * sizeof(int));
    if (!is_pattern) {
        *values = (double*)malloc(total_nnz * sizeof(double));
    } else {
        *values = NULL;
    }
    
    // Fill CSR arrays
    int* current_pos = (int*)malloc(rows * sizeof(int));
    for (int i = 0; i < rows; i++) {
        current_pos[i] = (*row_ptr)[i];
    }
    
    for (int i = 0; i < nonzeros; i++) {
        int r = temp_row[i];
        int c = temp_col[i];
        double val = is_pattern ? 1.0 : temp_val[i];
        
        int pos = current_pos[r]++;
        (*col_idx)[pos] = c;
        if (!is_pattern) (*values)[pos] = val;
        
        if (is_symmetric && r != c) {
            int pos2 = current_pos[c]++;
            (*col_idx)[pos2] = r;
            if (!is_pattern) (*values)[pos2] = val;
        }
    }
    
    free(current_pos);
    free(temp_row);
    free(temp_col);
    if (temp_val) free(temp_val);
    
    printf("[LOADER] CSR format ready: %d nodes, %d edges\n", rows, total_nnz);
    return 0;
}

void save_bfs_result(const char* filename, int* parent, int* level, int n, double time_ms, const char* algorithm) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "# BFS Results - %s\n", algorithm);
    fprintf(f, "# Time: %.6f ms\n", time_ms);
    fprintf(f, "# Format: Node | Parent | Level\n");
    fprintf(f, "# ------------------------------\n");
    
    int reachable = 0;
    for (int i = 0; i < n; i++) {
        fprintf(f, "%d %d %d\n", i, parent[i], level[i]);
        if (parent[i] != -1) reachable++;
    }
    
    fprintf(f, "# ------------------------------\n");
    fprintf(f, "# Reachable nodes: %d / %d\n", reachable, n);
    
    fclose(f);
    printf("[SAVE] Results saved to %s\n", filename);
}

void save_comparison(double classic_time, double graphblas_time, int consistent, int n) {
    FILE* f = fopen("bfs_comparison.txt", "w");
    if (!f) return;
    
    fprintf(f, "# BFS Performance Comparison\n");
    fprintf(f, "# ==========================\n");
    fprintf(f, "Graph size (nodes): %d\n", n);
    fprintf(f, "Classic BFS time: %.6f ms\n", classic_time);
    fprintf(f, "GraphBLAS BFS time: %.6f ms\n", graphblas_time);
    fprintf(f, "Speedup: %.2fx\n", classic_time / graphblas_time);
    fprintf(f, "Results consistent: %s\n", consistent ? "YES" : "NO");
    
    if (classic_time < graphblas_time) {
        fprintf(f, "\nClassic BFS is faster for this graph\n");
    } else {
        fprintf(f, "\nGraphBLAS BFS is faster for this graph\n");
    }
    
    fclose(f);
    printf("[SAVE] Comparison saved to bfs_comparison.txt\n");
}
