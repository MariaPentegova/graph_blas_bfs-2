#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "test_utils.h"
#include "../main/classic_bfs.h"

void test_classic_parent_bfs_correctness() {
    printf("\nTest Classic Parent BFS Correctness\n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    int n = csr->n;
    int* parent = (int*)malloc(n * sizeof(int));
    assert(parent != NULL);

    Graph* temp_g = create_test_graph();
    int start_vertex = find_max_degree_vertex(temp_g);
    delete_graph(temp_g);

    printf("  Стартовая вершина (макс. степень): %d\n", start_vertex);

    csr_parent_bfs(csr, start_vertex, parent);

    int all_visited = 1;
    for (int i = 0; i < n; i++) {
        if (parent[i] == -1) {
            all_visited = 0;
            break;
        }
    }
    assert(all_visited == 1);
    printf("Все вершины посещены\n");

    assert(parent[start_vertex] == start_vertex);
    printf("Корень указывает на себя\n");

    int no_self_loops = 1;
    for (int i = 0; i < n; i++) {
        if (i != start_vertex && parent[i] == i) {
            no_self_loops = 0;
            break;
        }
    }
    assert(no_self_loops == 1);
    printf("Нет петель у не корневых вершин\n");

    int expected_parent_0[5] = {0, 0, 1, 0, 1};

    if (start_vertex == 0) {
        for (int i = 0; i < n; i++) {
            assert(parent[i] == expected_parent_0[i]);
        }
        printf("Родители корректны для start=0\n");
    } else if (start_vertex == 1) {
        /* Вершина 3 находится на расстоянии 1 сразу от 0 и от 4 —
         * оба варианта parent[3] валидны для корректного BFS, конкретный
         * зависит от порядка обхода списка смежности (не от корректности
         * алгоритма). Раньше здесь было жёстко "parent[3] == 0", что
         * ломалось на вашем реальном порядке обхода, хотя parent[3] == 4
         * — такой же правильный BFS-родитель. */
        assert(parent[0] == 1);
        assert(parent[2] == 1);
        assert(parent[3] == 0 || parent[3] == 4);
        assert(parent[4] == 1);
        printf("  [OK] Родители корректны для start=1\n");
    }
    for (int i = 0; i < n; i++) {
        if (i != start_vertex && parent[i] != -1) {
            assert(parent[i] != i);
        }
    }
    printf("Все родители корректны\n");

    free(parent);
    delete_csr(csr);
    printf("Тест Classic Parent BFS пройден!\n");
}

void test_classic_multisource_bfs_correctness() {
    printf("\nTest Classic Multisource BFS Correctness\n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    int n = csr->n;
    int* parent = (int*)malloc(n * sizeof(int));
    assert(parent != NULL);

    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    assert(sources != NULL);

    int step = n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }

    printf("Источники: ");
    for (int i = 0; i < num_sources; i++) {
        printf("%d ", sources[i]);
    }
    printf("\n");

    csr_multisource_bfs(csr, sources, num_sources, parent);

    int all_visited = 1;
    for (int i = 0; i < n; i++) {
        if (parent[i] == -1) {
            all_visited = 0;
            break;
        }
    }
    assert(all_visited == 1);
    printf("Все вершины посещены\n");

    int all_sources_root = 1;
    for (int i = 0; i < num_sources; i++) {
        if (parent[sources[i]] != sources[i]) {
            all_sources_root = 0;
            break;
        }
    }
    assert(all_sources_root == 1);
    printf("Все источники указывают на себя\n");

    int no_self_loops = 1;
    for (int i = 0; i < n; i++) {
        int is_source = 0;
        for (int j = 0; j < num_sources; j++) {
            if (i == sources[j]) {
                is_source = 1;
                break;
            }
        }
        if (!is_source && parent[i] == i) {
            no_self_loops = 0;
            break;
        }
    }
    assert(no_self_loops == 1);
    printf("Нет петель у не корневых вершин\n");

    int expected_parent[5] = {0, 1, 2, 3, 1};

    for (int i = 0; i < n; i++) {
        if (i == 0 || i == 1 || i == 2 || i == 3) {
            assert(parent[i] == i);
        } else if (i == 4) {
            assert(parent[4] == 1 || parent[4] == 3);
        }
    }
    printf("Родители корректны для данного графа\n");

    for (int i = 0; i < n; i++) {
        if (parent[i] != -1) {
            if (parent[i] == i) {
                int is_source = 0;
                for (int j = 0; j < num_sources; j++) {
                    if (i == sources[j]) {
                        is_source = 1;
                        break;
                    }
                }
                assert(is_source == 1);
            }
        }
    }
    printf("Нет циклов в parent\n");

    free(parent);
    free(sources);
    delete_csr(csr);
    printf("Тест Classic Multisource BFS пройден!\n");
}

int main() {
    test_classic_parent_bfs_correctness();
    test_classic_multisource_bfs_correctness();

    printf("Все тесты Classic BFS пройдены!\n");
    return 0;
}
