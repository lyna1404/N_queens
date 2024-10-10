#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//parallelisation du probleme N queens avec pthread

typedef struct {
  uint32_t thread_id;
  uint32_t n_size;            // Number of queens on the NxN chess board
  uint32_t *queen_positions;  // Store queen positions on the board
  uint32_t *column;           // Store available column moves/attacks
  uint32_t *diagonal_up;      // Store available diagonal moves/attacks
  uint32_t *diagonal_down;
  uint32_t column_j;          // Stores column to place the next queen in
  uint64_t placements;        // Tracks total number queen placements
  int solutions;         // Tracks number of solutions
} Board;

typedef struct {
  int id;
  int start_row;
  int end_row;
  int n_queens;
} plage_rows;


// fonction à executer par les thread
void* workerThread(void* arg){
    plage_rows* thread_data =(plage_rows*)arg;
    int* result = malloc(sizeof(int));
    *result = 0;
    for (int i = thread_data->start_row; i<thread_data->end_row;i++){
    // chaque thread à sa propre board
    Board* board = NULL;
    board = malloc(sizeof(Board));
    board->thread_id = thread_data->id;
    initialize_board(thread_data->n_queens,board);
    set_queen(i,board);
    place_next_queen(board);
    printf("thread %d found %d solutions\n",thread_data->id,board->solutions);
    *result += board->solutions;
    smash_board(board);
    }
   //on retourne avec le nombre de resultats trouvés
   pthread_exit(result);
   
   
}

void initialize_board(int n_queens,Board* board) {
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

void set_queen(int row_i,Board* board) {
  board->queen_positions[board->column_j] = row_i;
  board->column[row_i] = 0;
  board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] = 0;
  board->diagonal_down[board->column_j + row_i] = 0;
  ++board->column_j;
  ++board->placements;
 
}


void place_next_queen(Board* board) {
  for (uint32_t row_i = 0; row_i < board->n_size; ++row_i) {
    if (square_is_free(row_i,board)) {
      set_queen(row_i,board);
      if (board->column_j == board->n_size) {
        board->solutions += 1;
        
      } else {
        place_next_queen(board);
      }
      remove_queen(row_i,board);
    }
  }
}




void remove_queen(int row_i,Board* board) {
  --board->column_j;
  board->diagonal_down[board->column_j + row_i] = 1;
  board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] = 1;
  board->column[row_i] = 1;
}

void smash_board(Board* board) {
  free(board->queen_positions);
  free(board);
}

int square_is_free(int row_i,Board* board) {
  return board->column[row_i] &
         board->diagonal_up[(board->n_size - 1) + (board->column_j - row_i)] &
         board->diagonal_down[board->column_j + row_i];
}

// fonction pour repartir le travail entre les threads 
void divide_work(int n_queens,int nthreads,pthread_t* threads, int* threadsIds){

  printf("number of queens : %d\n",n_queens);
  printf("number of threads : %d\n",nthreads);
  // si le nombre de threads >= nombre de queens alors chaque thread prend une ligne
  if(n_queens<=nthreads){
    for (int i = 0; i<nthreads;i++){
    threadsIds[i] = i;
    plage_rows* plage = malloc(sizeof(plage_rows));
    plage->id = i;
    plage->n_queens = n_queens;
    if(i>=n_queens){
    	 plage->start_row = 0;
         plage->end_row = 0;
    }
    else{
       plage->start_row = i;
       plage->end_row = i+1;
       pthread_create(&threads[i], NULL, workerThread,(void*)plage);
    }
    pthread_create(&threads[i], NULL, workerThread,(void*)plage);
    }
  }
  //sinon on répartit les lignes équitablement sur les threads
  else{
     int rows_per_thread = n_queens / nthreads;
     int extraqueens = n_queens % nthreads;
     int next_start = 0;
     for (int i = 0; i<nthreads;i++){
      int additional_row = 0;
      threadsIds[i] = i;
      if (extraqueens > 0){
        additional_row ++;
        extraqueens --;
      }
      
       plage_rows* plage = malloc(sizeof(plage_rows));
       plage->id = i;
       plage->n_queens = n_queens;
       plage->start_row = next_start;
       plage->end_row = next_start + rows_per_thread + additional_row;
       next_start = plage->end_row;
       pthread_create(&threads[i], NULL, workerThread,(void*)plage);

     }
     
  }
  
}


//fonction pour résoudre le probleme
double solve_nqueens(int n_queens,int numThreads){
    pthread_t threads[numThreads];
    int threadIds[numThreads];
    struct timeval start,end;
    gettimeofday(&start,NULL);
    // on commence par repartir le travail selon les threads
    divide_work(n_queens,numThreads,threads,threadIds);
    
    int* thread_solutions;
    int total_sum = 0;
    // on attend que tout les thread finissent leur travail en sommant les résultat obtenus
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], (void**)&thread_solutions);
        total_sum += *thread_solutions;
        
    }
    free(thread_solutions);
    gettimeofday(&end,NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long miliseconds = end.tv_usec - start.tv_usec;
    double t = seconds + miliseconds/1000000.0;
    printf("Time: %.6f\n", t);
    printf("Solution : %d \n",total_sum);
    return t;

}
// la fonction main sert à executer tous les cas demandé
int main() {
    FILE *file;
    file = fopen("output.txt","w");
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
