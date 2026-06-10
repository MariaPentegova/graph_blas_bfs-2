#ifndef CLASSIC_BFS_H
#define CLASSIC_BFS_H

typedef struct {
    int* parent;    // parent of each node (-1 if unreachable)
    int* level;     // level/distance from source (-1 if unreachable)
    int* visited;   // visited flag (1 if visited, 0 otherwise)
    int visited_count;  // number of visited nodes
} BFSResult;

// Single source BFS
BFSResult* classic_bfs(int n, int* row_ptr, int* col_idx, int source);

// Multi-source BFS
void classic_bfs_multisource(int n, int* row_ptr, int* col_idx, 
                              int* sources, int num_sources, BFSResult* result);

// Free BFS result
void free_bfs_result(BFSResult* result);

#endif
