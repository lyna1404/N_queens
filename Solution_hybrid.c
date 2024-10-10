#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>
#include <mpi.h>



typedef struct {
  uint32_t n_size;            // Number of queens on the NxN chess board
  uint32_t *queen_positions;  // Store queen positions on the board
  uint32_t *column;           // Store available column moves/attacks
  uint32_t *diagonal_up;      // Store available diagonal moves/attacks
  uint32_t *diagonal_down;
  uint32_t column_j;          // Stores column to place the next queen in
  uint64_t placements;        // Tracks total number queen placements
  int solutions;              // Tracks number of solutions
} Board;

void initialize_board(uint32_t n_queens, Board *board) {
  if (n_queens < 1) {
    fprintf(stderr, "The number of queens must be greater than 0.\n");
    exit(EXIT_FAILURE);
  }

  if (board == NULL) {
    fprintf(stderr, "Memory allocation failed for chess board.\n");
    exit(EXIT_FAILURE);
  }

  const uint32_t diagonal_size = 2 * n_queens - 1;
  const uint32_t total_size = 2 * (n_queens + diagonal_size);
  board->queen_positions = malloc(sizeof(uint32_t) * total_size);
  if (board->queen_positions == NULL) {
    fprintf(stderr, "Memory allocation failed for the chess board arrays.\n");
    exit(EXIT_FAILURE);
  }
  board->column = &board->queen_positions[n_queens];
  board->diagonal_up = &board->column[n_queens];
  board->diagonal_down = &board->diagonal_up[diagonal_size];

  board->n_size = n_queens;
  for (uint32_t i = 0; i < n_queens; ++i) {
    board->queen_positions[i] = 0;
  }
  for (uint32_t i = n_queens; i < total_size; ++i) {
    board->queen_positions[i] = 1;
  }
  board->column_j = 0;
  board->placements = 0;
  board->solutions = 0;
}

void set_queen(uint32_t row_i, Board *board) {
  board->queen_positions[board->column_j] = row_i;
  board->column[row_i] = 0;
  board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] = 0;
  board->diagonal_down[board->column_j + row_i] = 0;
  ++board->column_j;
  ++board->placements;
}

void remove_queen(uint32_t row_i, Board *board) {
  --board->column_j;
  board->diagonal_down[board->column_j + row_i] = 1;
  board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] = 1;
  board->column[row_i] = 1;
  board->queen_positions[board->column_j] = 0;
  --board->placements;
}

int square_is_free(uint32_t row_i, const Board *board) {
  return board->column[row_i] &&
         board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] &&
         board->diagonal_down[board->column_j + row_i];
}

void place_next_queen(Board *board) {
  if (board->column_j == board->n_size) {
    #pragma omp atomic
    board->solutions++;
  } else {
    for (uint32_t row_i = 0; row_i < board->n_size; ++row_i) {
      if (square_is_free(row_i, board)) {
        set_queen(row_i, board);
        place_next_queen(board);
        remove_queen(row_i, board);
      }
    }
  }
}

//chaque noeud du cluster résout le proleme en parrallelisant avec OpenMP
int solve_nqueens(int n_queens, int start_row, int end_row, int nthreads) {

  double start_time, end_time;
  printf("number of queens : %d\n",n_queens);
  printf("number of threads : %d\n",nthreads);
  int total_sum = 0;
  #pragma omp parallel num_threads(nthreads)
  {
    int thread_solutions = 0;

    #pragma omp for schedule(static)
    for (int i = start_row; i < end_row; ++i) {
      Board* board = malloc(sizeof(Board));
      initialize_board(n_queens, board);
      set_queen(i, board);
      place_next_queen(board);
      thread_solutions += board->solutions;
      free(board->queen_positions);
      free(board);
    }

    #pragma omp atomic
    total_sum += thread_solutions;
  }
  return total_sum;
}

//fonction pour répartir le travail et synchroniser les résultats
void work_function(int n_queens, int nthreads,FILE* file){
  int rank, num_processes;
  int solutions = 0;
  int worker_solutions = 0;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
  // erreur si le nombre de process > 3 car on a un master et 2 workers
  if (num_processes < 3) {
    fprintf(stderr, "Error: This program requires at least 3 processes.\n");
    MPI_Finalize();
    
  }
  else{
  if (rank == 0) { 
    // Master node
      double start, end, elapsed_time;
      solutions = 0;
      start = MPI_Wtime();
      // Diviser les lignes entre les noeuds workers
      int rows_per_worker = n_queens / (num_processes - 1);
      int extra_rows = n_queens % (num_processes - 1);
      int start_row = 0;
      int end_row = rows_per_worker;

      for (int worker_rank = 1; worker_rank < num_processes; worker_rank++) {
        int additional_row = 0;
        if(extra_rows!=0){additional_row ++; extra_rows--;}
        // envoyer les plages de travail aux xorkers
        int worker_data[3] = {n_queens, start_row, end_row};
        MPI_Send(worker_data, 3, MPI_INT, worker_rank, 0, MPI_COMM_WORLD);
        start_row = end_row;
        end_row += rows_per_worker+additional_row;
      }

      // recevoir les résultats from les noeuds workers et les sommer
      for (int worker_rank = 1; worker_rank < num_processes; worker_rank++) {
        worker_solutions = 0;
        MPI_Recv(&worker_solutions, 1, MPI_INT, worker_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        solutions += worker_solutions;
      }
      end = MPI_Wtime();
      elapsed_time = end - start;
      // Afficher les résultats
      printf("Total solutions for %d queens: %d\n", n_queens, solutions);
      printf("Time %.6f\n", elapsed_time);
      fprintf(file,"queens : %d,threads : %d time : %.6f\n",n_queens,nthreads,elapsed_time);
      
  } else {
    // Worker nodes
    int worker_data[3];
    MPI_Recv(worker_data, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int n_queens = worker_data[0];
    int start_row = worker_data[1];
    int end_row = worker_data[2];
    //résoudre le probleme et envoyer les résultats au noeuds maitre
    worker_solutions = solve_nqueens(n_queens, start_row, end_row,nthreads);
    MPI_Send(&worker_solutions, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }

  
}
}
// Executer les differents cas demandés
int main(int argc, char *argv[]) {
  FILE *file;
  file = fopen("output3.txt","w");
  int tailles[]= {4,8,16,17};
  int threads[]={4,8,16,32};
  
  MPI_Init(&argc, &argv);
  work_function(8,8,file);
  /*for(int i=0;i<4;i++){
      
      work_function(tailles[i],8,file);
      
      
    }
    for(int i=0;i<4;i++){
     
      work_function(17,threads[i],file);
      
    }*/
  fclose(file);  
  MPI_Finalize();
  return 0;
}
