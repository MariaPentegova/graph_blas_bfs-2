#include <assert.h>
#include <stdio.h>
#include "test_common.h"
#include "utils.h"

#define TEST_FILE "test_graph.mtx"

static void write_test_file(const char* text) {
    FILE* f = fopen(TEST_FILE, "w");
    assert(f != NULL);
    fputs(text, f);
    fclose(f);
}

static CSRMatrix* load_test_csr(const char* text) {
    write_test_file(text);
    CSRMatrix* csr = load_csr(TEST_FILE);
    remove(TEST_FILE);
    return csr;
}

static void test_load_symmetric(void) {
    CSRMatrix* csr = load_test_csr(
        "%%MatrixMarket matrix coordinate pattern symmetric\n"
        "% comment line\n"
        "3 3 3\n"
        "2 1\n"
        "3 2\n"
        "3 3\n");
    assert(csr != NULL);

    const int expected_row_ptr[4] = {0, 1, 3, 5};
    const int expected_col_idx[5] = {1, 0, 2, 1, 2};
    assert(csr->n == 3);
    assert(arrays_equal(csr->row_ptr, expected_row_ptr, 4));
    assert(arrays_equal(csr->col_idx, expected_col_idx, 5));

    delete_csr(csr);
    printf("PASSED: loading an undirected graph\n");
}

static void test_load_general(void) {
    CSRMatrix* csr = load_test_csr(
        "%%MatrixMarket matrix coordinate pattern general\n"
        "3 3 2\n"
        "1 2\n"
        "3 1\n");
    assert(csr != NULL);

    const int expected_row_ptr[4] = {0, 1, 1, 2};
    const int expected_col_idx[2] = {1, 0};
    assert(csr->n == 3);
    assert(arrays_equal(csr->row_ptr, expected_row_ptr, 4));
    assert(arrays_equal(csr->col_idx, expected_col_idx, 2));

    delete_csr(csr);
    printf("PASSED: loading a directed graph\n");
}

static void test_load_sorts_rows_and_removes_duplicates(void) {
    CSRMatrix* csr = load_test_csr(
        "%%MatrixMarket matrix coordinate pattern general\n"
        "3 3 5\n"
        "1 3\n"
        "1 2\n"
        "1 3\n"
        "2 1\n"
        "1 2\n");
    assert(csr != NULL);

    const int expected_row_ptr[4] = {0, 2, 3, 3};
    const int expected_col_idx[3] = {1, 2, 0};
    assert(arrays_equal(csr->row_ptr, expected_row_ptr, 4));
    assert(arrays_equal(csr->col_idx, expected_col_idx, 3));

    delete_csr(csr);
    printf("PASSED: neighbours are sorted, duplicate edges are removed\n");
}

static void test_load_invalid_files(void) {
    const char* invalid_files[] = {
        "3 3 1\n1 2\n",
        "%%MatrixMarket matrix coordinate real general\n3 3 1\n1 2 0.5\n",
        "%%MatrixMarket matrix coordinate pattern general\n3 4 1\n1 2\n",
        "%%MatrixMarket matrix coordinate pattern general\n3 3 1\n1 4\n",
        "%%MatrixMarket matrix coordinate pattern general\n3 3 2\n1 2\n"
    };
    for (size_t i = 0; i < sizeof(invalid_files) / sizeof(invalid_files[0]); i++) {
        CSRMatrix* csr = load_test_csr(invalid_files[i]);
        assert(csr == NULL);
    }

    CSRMatrix* missing = load_csr("no_such_file.mtx");
    assert(missing == NULL);

    printf("PASSED: invalid files are rejected\n");
}

static void test_max_degree_vertex(void) {
    CSRMatrix* undirected = create_test_csr(UNDIRECTED_GRAPH);
    CSRMatrix* directed = create_test_csr(DIRECTED_GRAPH);

    int undirected_vertex = find_max_degree_vertex(undirected);   // степени 2, 3, 1, 2, 2
    int directed_vertex = find_max_degree_vertex(directed);       // исходящие: 1, 1, 0, 1
    assert(undirected_vertex == 1);
    assert(directed_vertex == 0);

    delete_csr(undirected);
    delete_csr(directed);
    printf("PASSED: max degree vertex\n");
}

int main(void) {
    test_load_symmetric();
    test_load_general();
    test_load_sorts_rows_and_removes_duplicates();
    test_load_invalid_files();
    test_max_degree_vertex();

    printf("All loading tests passed\n");
    return 0;
}
