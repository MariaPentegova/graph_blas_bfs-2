#include <assert.h>
#include <stdio.h>
#include "test_common.h"
#include "utils.h"

static void test_check_detects_errors(void) {
    CSRMatrix* csr = create_test_csr(UNDIRECTED_GRAPH);
    int depth[5];

    const int correct[5] = {1, 1, 1, 0, 1};
    const int missing_edge[5] = {1, 1, 1, 2, 1};
    const int cycle[5] = {3, 1, 1, 0, 1};
    const int lost_vertex[5] = {1, 1, 1, -1, 1};
    const int not_shortest[5] = {0, 0, 1, 4, 1};

    assert(bfs_tree_depths(csr, correct, depth) == 0);
    assert(bfs_tree_depths(csr, missing_edge, depth) == -1);
    assert(bfs_tree_depths(csr, cycle, depth) == -1);
    assert(bfs_tree_depths(csr, lost_vertex, depth) == -1);
    assert(bfs_tree_depths(csr, not_shortest, depth) == -1);

    delete_csr(csr);
    printf("PASSED: the result check detects incorrect trees\n");
}

int main(void) {
    test_check_detects_errors();

    printf("All result check tests passed\n");
    return 0;
}
