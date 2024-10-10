#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>



typedef struct {
  uint32_t thread_id;
  uint32_t n_size;            // Number of queens on the NxN chess board
  uint32_t *queen_positions;  // Store queen positions on the board
  uint32_t *column;           // Store available column moves/attacks
  uint32_t *diagonal_up;      // Store available diagonal moves/attacks
  uint32_t *diagonal_down;
  uint32_t column_j;          // Stores column to place the next queen in
  uint64_t placements;        // Tracks total number queen placements
  int solutions;              // Tracks number of solutions
} Board;

void initialize_board(int n_queens, Board* board) {
  if (n_queens < 1) {
    fprintf(stderr, "The number of queens must be greater than 0.\n");
    exit(EXIT_SUCCESS);
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
    free(board);
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

void set_queen(int row_i, Board* board) {
  board->queen_positions[board->column_j] = row_i;
  board->column[row_i] = 0;
  board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] = 0;
  board->diagonal_down[board->column_j + row_i] = 0;
  ++board->column_j;
  ++board->placements;
}

void place_next_queen(Board* board) {
  for (uint32_t row_i = 0; row_i < board->n_size; ++row_i) {
    if (square_is_free(row_i, board)) {
      set_queen(row_i, board);
      if (board->column_j == board->n_size) {
        #pragma omp atomic
        board->solutions += 1;
      } else {
        place_next_queen(board);
      }
      remove_queen(row_i, board);
    }
  }
}

void remove_queen(int row_i, Board* board) {
  --board->column_j;
  board->diagonal_down[board->column_j + row_i] = 1;
  board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] = 1;
  board->column[row_i] = 1;
}

void smash_board(Board* board) {
  free(board->queen_positions);
  free(board);
}

int square_is_free(int row_i, Board* board) {
  return board->column[row_i] &
         board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] &
         board->diagonal_down[board->column_j + row_i];
}
//fonction pour résoudre le probleme
double solve_nqueens(int n_queens,int nthreads) {
  double start_time, end_time;
  printf("number of queens : %d\n",n_queens);
  printf("number of threads : %d\n",nthreads);
  start_time = omp_get_wtime();

  int total_sum = 0;
  // region parrallele en spécifiant le nombre de threads
  #pragma omp parallel num_threads(nthreads)
  {
    int thread_solutions = 0;
    //parralleliser la boucle en fonction des threads
    #pragma omp for schedule(static)
    for (int i = 0; i < n_queens; ++i) {
      Board* board = malloc(sizeof(Board));
      initialize_board(n_queens, board);
      set_queen(i, board);
      place_next_queen(board);
      thread_solutions += board->solutions;
      smash_board(board);
    }
    #pragma omp atomic
    total_sum += thread_solutions;
  }

  end_time = omp_get_wtime();
  printf("Time: %.6f\n", end_time - start_time);
  printf("Solution: %d\n", total_sum);
  return (end_time - start_time);
}

int main() {
  FILE *file;
    file = fopen("output2.txt","w");
    double* time1 = malloc(5*sizeof(double));
    double* time2 = malloc(5*sizeof(double));
    int tailles[]= {1,4,8,16,17};
    int threads[]={1,4,8,16,32};
    time1[0] = solve_nqueens(8,8);
    fprintf(file,"queens : %d,time : %.6f\n",8,time1[0]);
    /*for(int i=0;i<5;i++){
    	time1[i] = solve_nqueens(tailles[i],8);
    	fprintf(file,"queens : %d,time : %.6f\n",tailles[i],time1[i]);
    }
    for(int i=0;i<5;i++){
    	time2[i] = solve_nqueens(17,threads[i]);
    	fprintf(file,"threads: %d,time : %.6f\n",threads[i],time2[i]);
    }*/
    fclose(file);

  return 0;
}

