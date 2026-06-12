#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "classic_bfs.h"

void test_line_graph() {
    printf("Test 1: BFS on line graph (0-1-2-3)\n");
    
    int row_ptr[] = {0, 1, 3, 5, 6};
    int col_idx[] = {1, 0, 2, 1, 3, 2};
    int n = 4;
    
    BFSResult* result = classic_bfs(n, row_ptr, col_idx, 0);
    
    assert(result->parent[0] == 0);
    assert(result->parent[1] == 0);
    assert(result->parent[2] == 1);
    assert(result->parent[3] == 2);
    
    assert(result->level[0] == 0);
    assert(result->level[1] == 1);
    assert(result->level[2] == 2);
    assert(result->level[3] == 3);
    
    assert(result->visited_count == 4);
    
    free_bfs_result(result);
    printf("  OK\n");
}

void test_disconnected_graph() {
    printf("Test 2: BFS on disconnected graph\n");
    
    int row_ptr[] = {0, 1, 2, 2, 3, 4};
    int col_idx[] = {1, 0, 3, 4, 3};
    int n = 5;
    
    BFSResult* result = classic_bfs(n, row_ptr, col_idx, 0);
    
    assert(result->parent[0] == 0);
    assert(result->parent[1] == 0);
    assert(result->parent[2] == -1);
    assert(result->parent[3] == -1);
    assert(result->parent[4] == -1);
    
    assert(result->level[0] == 0);
    assert(result->level[1] == 1);
    assert(result->level[2] == -1);
    assert(result->level[3] == -1);
    assert(result->level[4] == -1);
    
    assert(result->visited_count == 2);
    
    free_bfs_result(result);
    printf("  OK\n");
}

void test_single_node() {
    printf("Test 3: BFS on single node\n");
    
    int row_ptr[] = {0, 0};
    int col_idx[] = {};
    int n = 1;
    
    BFSResult* result = classic_bfs(n, row_ptr, col_idx, 0);
    
    assert(result->parent[0] == 0);
    assert(result->level[0] == 0);
    assert(result->visited_count == 1);
    
    free_bfs_result(result);
    printf("  OK\n");
}

int main() {
    test_line_graph();
    test_disconnected_graph();
    test_single_node();
    
    printf("All classic BFS tests passed\n");
    return 0;
}