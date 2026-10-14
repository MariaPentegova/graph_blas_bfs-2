#include <algorithm>
#include <cstdio>
#include <random>
#include <time.h>
#include <utility>
#include <vector>

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/visitors.hpp>

#include "utils.h"
#include "classic_bfs.h"
#include "graphblas_bfs.h"

typedef boost::compressed_sparse_row_graph<boost::directedS, boost::no_property,
                                           boost::no_property, boost::no_property,
                                           int, int> BGLGraph;

static double now_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static double microseconds_since(double start) {
    return (now_seconds() - start) * 1e6;
}

static double average(const std::vector<double>& values) {
    double sum = 0.0;
    for (double x : values) {
        sum += x;
    }
    return sum / (double)values.size();
}

static void print_times(const char* name, const std::vector<double>& times) {
    double min = *std::min_element(times.begin(), times.end());
    double max = *std::max_element(times.begin(), times.end());
    std::printf("%-28s | %12.0f | %12.0f | %12.0f\n", name, average(times), min, max);
}

static BGLGraph build_bgl_graph(const CSRMatrix* csr) {
    std::vector<std::pair<int, int>> edges;
    edges.reserve((size_t)csr->row_ptr[csr->n]);
    for (int u = 0; u < csr->n; u++) {
        for (int i = csr->row_ptr[u]; i < csr->row_ptr[u + 1]; i++) {
            edges.push_back(std::make_pair(u, csr->col_idx[i]));
        }
    }
    return BGLGraph(boost::edges_are_sorted, edges.begin(), edges.end(), csr->n);
}

static bool same_result(const CSRMatrix* csr, const std::vector<int>& parent_a,
                        const std::vector<int>& parent_b) {
    std::vector<int> depth_a(csr->n);
    std::vector<int> depth_b(csr->n);
    return bfs_tree_depths(csr, parent_a.data(), depth_a.data()) == 0 &&
           bfs_tree_depths(csr, parent_b.data(), depth_b.data()) == 0 &&
           depth_a == depth_b;
}

static bool finish_graphblas_run(void* result, bool is_last_run, std::vector<int>& parent) {
    bool ok = true;
    if (is_last_run) {
        ok = graphblas_parent_to_array(result, (int)parent.size(), parent.data()) == 0;
    }
    graphblas_free_vector(result);
    return ok;
}

/* Characteristics of a BFS traversal, computed from a checked parent tree.
 * They do not depend on the implementation and are computed outside the timing. */
struct TraversalStats {
    long long reached;        /* vertices reached from the sources                     */
    int levels;               /* number of BFS levels (maximum depth + 1)              */
    long long max_frontier;   /* largest number of vertices on one level               */
    long long edges;          /* out-edges of reached vertices = edges examined by BFS */
};

static bool traversal_stats(const CSRMatrix* csr, const std::vector<int>& parent,
                            TraversalStats* stats) {
    std::vector<int> depth(csr->n);
    if (bfs_tree_depths(csr, parent.data(), depth.data()) != 0) {
        return false;
    }
    std::vector<long long> level_size;
    stats->reached = 0;
    stats->edges = 0;
    for (int v = 0; v < csr->n; v++) {
        if (depth[v] < 0) {
            continue;
        }
        if ((size_t)depth[v] >= level_size.size()) {
            level_size.resize((size_t)depth[v] + 1, 0);
        }
        level_size[(size_t)depth[v]]++;
        stats->reached++;
        stats->edges += csr->row_ptr[v + 1] - csr->row_ptr[v];
    }
    stats->levels = (int)level_size.size();
    stats->max_frontier = level_size.empty() ? 0
                        : *std::max_element(level_size.begin(), level_size.end());
    return true;
}

static void print_stats(const char* mode, const TraversalStats& st, int n,
                        double t_classic, double t_graphblas) {
    std::printf("  %s:\n", mode);
    std::printf("    reached vertices:   %lld (%.2f%% of all)\n", st.reached, 100.0 * st.reached / n);
    std::printf("    levels:             %d\n", st.levels);
    std::printf("    max level size:     %lld\n", st.max_frontier);
    std::printf("    edges examined:     %lld\n", st.edges);
    std::printf("    edges per level:    %.0f\n", st.levels > 0 ? (double)st.edges / st.levels : 0.0);
    std::printf("    MTEPS Classic:      %.1f\n", (double)st.edges / t_classic);
    std::printf("    MTEPS GraphBLAS:    %.1f\n", (double)st.edges / t_graphblas);
}

/* Multisource BFS sources: num_sources distinct vertices with out-degree > 0,
 * chosen uniformly at random with a fixed seed (reproducible, as in Graph500). */
static bool choose_sources(const CSRMatrix* csr, int* sources, int num_sources) {
    std::vector<int> candidates;
    for (int v = 0; v < csr->n; v++) {
        if (csr->row_ptr[v + 1] > csr->row_ptr[v]) {
            candidates.push_back(v);
        }
    }
    if ((int)candidates.size() < num_sources) {
        return false;
    }
    std::mt19937 rng(12345);
    for (int i = 0; i < num_sources; i++) {
        std::uniform_int_distribution<int> pick(i, (int)candidates.size() - 1);
        std::swap(candidates[(size_t)i], candidates[(size_t)pick(rng)]);
        sources[i] = candidates[(size_t)i];
    }
    return true;
}

static void print_check(const char* description, bool ok) {
    std::printf("  %-40s %s\n", description, ok ? "OK" : "MISMATCH");
}

static bool run_benchmark(const char* graph_file, int num_runs) {
    std::printf("\n=== %s ===\n", graph_file);
    std::printf("Loading the graph...\n");

    CSRMatrix* csr = load_csr(graph_file);
    if (csr == NULL) {
        return false;
    }

    const int n = csr->n;
    const int start = find_max_degree_vertex(csr);
    constexpr int num_sources = 4;
    int sources[num_sources];
    if (!choose_sources(csr, sources, num_sources)) {
        std::fprintf(stderr, "Error: not enough vertices with outgoing edges\n");
        delete_csr(csr);
        return false;
    }

    std::printf("GraphBLAS threads: %d (Classic BFS is sequential)\n", graphblas_num_threads());
    std::printf("Average out-degree: %.2f\n", (double)csr->row_ptr[n] / n);
    std::printf("Start vertex (max out-degree): %d, degree %d\n",
                start, csr->row_ptr[start + 1] - csr->row_ptr[start]);
    std::printf("Sources for multisource BFS (random, out-degree > 0): %d %d %d %d\n",
                sources[0], sources[1], sources[2], sources[3]);

    double t = now_seconds();
    const BGLGraph bgl_graph = build_bgl_graph(csr);
    std::printf("BGL graph construction (not measured): %.6f sec\n", now_seconds() - t);

    t = now_seconds();
    void* gb_matrix = NULL;
    if (graphblas_build_matrix(csr, &gb_matrix) != 0) {
        delete_csr(csr);
        return false;
    }
    std::printf("GraphBLAS matrix construction (not measured): %.6f sec\n", now_seconds() - t);

    std::vector<int> classic_parent(n);
    std::vector<int> bgl_parent(n);
    std::vector<int> classic_multi_parent(n);
    std::vector<int> gb_parent(n);
    std::vector<int> gb_multi_parent(n);

    std::vector<double> t_classic;
    std::vector<double> t_bgl;
    std::vector<double> t_classic_multi;
    std::vector<double> t_gb;
    std::vector<double> t_gb_multi;
    int errors = 0;

    std::printf("Running BFS: 1 warm-up run + %d measured runs of each algorithm...\n", num_runs);

    for (int run = 0; run <= num_runs; run++) {
        const bool warmup = (run == 0);
        const bool last_run = (run == num_runs);
        void* gb_result = NULL;

        t = now_seconds();
        if (csr_parent_bfs(csr, start, classic_parent.data()) != 0) errors++;
        if (!warmup) t_classic.push_back(microseconds_since(t));

        t = now_seconds();
        std::fill(bgl_parent.begin(), bgl_parent.end(), -1);
        bgl_parent[start] = start;
        boost::breadth_first_search(bgl_graph, start,
            boost::visitor(boost::make_bfs_visitor(
                boost::record_predecessors(bgl_parent.data(), boost::on_tree_edge()))));
        if (!warmup) t_bgl.push_back(microseconds_since(t));

        t = now_seconds();
        if (csr_multisource_bfs(csr, sources, num_sources, classic_multi_parent.data()) != 0) errors++;
        if (!warmup) t_classic_multi.push_back(microseconds_since(t));

        t = now_seconds();
        if (graphblas_parent_bfs(csr, gb_matrix, start, &gb_result) != 0) errors++;
        if (!warmup) t_gb.push_back(microseconds_since(t));
        if (!finish_graphblas_run(gb_result, last_run, gb_parent)) errors++;

        t = now_seconds();
        if (graphblas_multisource_parent_bfs(csr, gb_matrix, sources, num_sources,
                                             &gb_result) != 0) errors++;
        if (!warmup) t_gb_multi.push_back(microseconds_since(t));
        if (!finish_graphblas_run(gb_result, last_run, gb_multi_parent)) errors++;
    }

    std::printf("\nResults (microseconds, %d runs)\n", num_runs);
    std::printf("%-28s | %12s | %12s | %12s\n", "Algorithm", "Avg", "Min", "Max");
    print_times("Classic Parent BFS", t_classic);
    print_times("Boost BGL Parent BFS", t_bgl);
    print_times("Classic Multisource BFS", t_classic_multi);
    print_times("GraphBLAS Parent BFS", t_gb);
    print_times("GraphBLAS Multisource BFS", t_gb_multi);

    std::printf("\nSpeedup S = T_Classic / T_GraphBLAS (S > 1 means GraphBLAS is faster)\n");
    std::printf("  single-source: %.4f\n", average(t_classic) / average(t_gb));
    std::printf("  multisource:   %.4f\n", average(t_classic_multi) / average(t_gb_multi));

    const bool no_errors = (errors == 0);
    const bool classic_vs_bgl = same_result(csr, classic_parent, bgl_parent);
    const bool classic_vs_gb = same_result(csr, classic_parent, gb_parent);
    const bool classic_vs_gb_multi = same_result(csr, classic_multi_parent, gb_multi_parent);

    TraversalStats single_stats;
    TraversalStats multi_stats;
    const bool single_stats_ok = traversal_stats(csr, classic_parent, &single_stats);
    const bool multi_stats_ok = traversal_stats(csr, classic_multi_parent, &multi_stats);
    std::printf("\nTraversal characteristics (MTEPS = edges examined / average time)\n");
    if (single_stats_ok) {
        print_stats("single-source", single_stats, n, average(t_classic), average(t_gb));
    }
    if (multi_stats_ok) {
        print_stats("multisource", multi_stats, n, average(t_classic_multi), average(t_gb_multi));
    }

    std::printf("\nCorrectness checks\n");
    print_check("All runs finished without errors", no_errors);
    print_check("Classic vs BGL (single-source)", classic_vs_bgl);
    print_check("Classic vs GraphBLAS (single-source)", classic_vs_gb);
    print_check("Classic vs GraphBLAS (multisource)", classic_vs_gb_multi);

    graphblas_free_matrix(gb_matrix);
    delete_csr(csr);
    return no_errors && classic_vs_bgl && classic_vs_gb && classic_vs_gb_multi;
}

int main(int argc, char* argv[]) {
    constexpr int num_runs = 10;
    const char* default_graphs[] = {
        "../graphs/cit-Patents.mtx",
        "../graphs/com-Amazon.mtx",
        "../graphs/com-Orkut.mtx",
        "../graphs/roadNet-CA.mtx",
        "../graphs/road_usa.mtx",
        "../graphs/soc-LiveJournal1.mtx",
        "../graphs/web-Google.mtx"
    };

    const char* const* graphs = default_graphs;
    int num_graphs = (int)(sizeof(default_graphs) / sizeof(default_graphs[0]));
    if (argc >= 2) {
        graphs = argv + 1;
        num_graphs = argc - 1;
    }

    std::setvbuf(stdout, NULL, _IOLBF, 0);

    if (graphblas_init() != 0) {
        return 1;
    }

    bool ok = true;
    for (int i = 0; i < num_graphs; i++) {
        if (!run_benchmark(graphs[i], num_runs)) {
            ok = false;
        }
    }

    graphblas_finalize();
    return ok ? 0 : 1;
}
