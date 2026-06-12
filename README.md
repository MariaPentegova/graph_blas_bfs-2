# Сравнение BFS: классическая реализация (CSR) и GraphBLAS

## О проекте

В данном проекте реализованы и протестированы два варианта алгоритма обхода графа в ширину (BFS):

* **Классический BFS**: на основе очереди и CSR-представления графа.
* **BFS на основе линейной алгебры**: через библиотеку **SuiteSparse:GraphBLAS**.

---

## Структура проекта

```text
.
├── src/
│   ├── main/
│   │   ├── classic_bfs.c
│   │   ├── classic_bfs.h
│   │   ├── graphblas_bfs.c
│   │   ├── graphblas_bfs.h
│   │   ├── utils.c
│   │   ├── utils.h
│   │   └── main.c
│   └── tests/
│       ├── test_classic_bfs.c
│       └── test_graphblas_bfs.c
├── graphs/
├── CMakeLists.txt
└── README.md

