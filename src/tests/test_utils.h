#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "../main/utils.h"

Graph* create_test_graph();

CSRMatrix* create_test_csr();

int check_all_visited(int* level, int n);
int compare_levels(int* level1, int* level2, int n);
int check_parent_correctness(int* parent, int n, int start_vertex);

#endif
