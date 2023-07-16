# N_queens
## Parallelized N-Queens Problem Solvers
This project focuses on solving the N-Queens problem using parallel programming techniques to improve performance and scalability. Four parallelization models are explored: Pthreads, MPI, OpenMP, and a hybrid approach combining MPI and OpenMP (MPI/OpenMP).

## Introduction
The N-Queens problem involves placing N chess queens on an N x N chessboard, such that no two queens can threaten each other. The goal of this project is to develop parallel algorithms that leverage the capabilities of different parallel programming models to solve the N-Queens problem efficiently.

## Features
### Pthreads-based Parallel Solver:

- Utilizes Pthreads to create a multi-threaded N-Queens solver.
- Divides the search space into smaller subproblems for parallel execution.
- Implements thread synchronization techniques to ensure correctness.
- Measures and analyzes performance gains achieved through parallelization.
### MPI-based Parallel Solver:

- Utilizes the Message Passing Interface (MPI) for distributed memory parallelism.
- Divides the problem into subproblems that are independently solved by different processes.
- Implements communication routines to exchange information among processes.
- Evaluates the scalability and efficiency of the MPI-based solution.
### OpenMP-based Parallel Solver:

- Implements an N-Queens solver using OpenMP for shared memory parallelism.
- Utilizes task parallelism to divide the problem into smaller tasks executed by multiple threads.
- Employs OpenMP directives and constructs for thread synchronization and workload balancing.
- Measures the performance improvements achieved through OpenMP parallelization.
### MPI/OpenMP Hybrid Solver:

- Combines the strengths of MPI and OpenMP for hybrid parallelism.
- Distributes the problem across multiple processes using MPI and uses OpenMP within each process for intra-node parallelism.
- Optimizes partitioning and task distribution across processes and threads.
- Assesses the overall performance gains achieved by the hybrid approach.
- Needs to be compiled on a virtual cluster composed of 2 worker nodes and a master node.









