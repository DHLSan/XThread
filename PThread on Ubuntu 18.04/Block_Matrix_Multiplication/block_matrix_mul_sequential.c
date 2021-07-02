#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define BILLION  1E9;

/////////////////////////////

#define NUMOFCORE 1
#define MATRIX_SIZE 1024

/////////////////////////////

#define BLOCK_SIZE 16

float matrix_a[MATRIX_SIZE][MATRIX_SIZE];
float matrix_b[MATRIX_SIZE][MATRIX_SIZE];
float matrix_c[MATRIX_SIZE][MATRIX_SIZE];

void core_bmm(int core_number){

    /****************************************/

    float numofcore = NUMOFCORE;
    int matrix_size =  MATRIX_SIZE;

    int start_blocked_row = (matrix_size/numofcore)*core_number / BLOCK_SIZE;
    int end_blocked_row = (matrix_size/numofcore)*(core_number+1) / BLOCK_SIZE;

    //printf("\ncore: %d | start_block = %d | end_block = %d",core_number,start_blocked_row,end_blocked_row);

    // i indicates blocked_row
    for (int i = start_blocked_row; i < end_blocked_row; i++) {

        // j indicates blocked_column
        for (int j = 0; j < matrix_size / BLOCK_SIZE; j++) {


            // k indicates kth block on given i,j pair
            for(int k = 0; k < matrix_size / BLOCK_SIZE; k++){

     
                // mul inner matrix
                for (int x = 0; x < BLOCK_SIZE; x++) {
                    for (int y = 0; y < BLOCK_SIZE; y++) {
                        float temp = 0;
                        for (int z = 0; z < BLOCK_SIZE; z++) {
                            temp += matrix_a[i*BLOCK_SIZE + x][k*BLOCK_SIZE + z] * matrix_b[k*BLOCK_SIZE + z][j*BLOCK_SIZE + y];
                        }
                        matrix_c[i*BLOCK_SIZE + x][j*BLOCK_SIZE + y] = matrix_c[i*BLOCK_SIZE + x][j*BLOCK_SIZE + y] + temp;
                    }
                }

            }



        }

	}
}

void int_gemm(){
/*
    // create matrix_A
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            matrix_a[i][j] = i*MATRIX_SIZE + j;
        }
    }

    // create matrix_B
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            matrix_b[i][j] = i*MATRIX_SIZE + j;
        }
    }
*/
    // create matrix_C
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            matrix_c[i][j] = 0;
        }
    }

}

void print_gemm(){

    printf("\n");
    printf("numofcore: %d \n",(int) NUMOFCORE);
    printf("matrix_size: %d \n",(int) MATRIX_SIZE);
    printf("\n");
/*
    // print matrix_a
    //printf("matrix_a\n");
    for(int i=0;i<MATRIX_SIZE;i++){
        for(int j=0;j<MATRIX_SIZE;j++){ ;
            //printf("%d |", matrix_a[i][j]);
        }
        //printf("\n");
    }

    // print matrix_b
    //printf("matrix_b\n");
    for(int i=0;i<MATRIX_SIZE;i++){
        for(int j=0;j<MATRIX_SIZE;j++){ ;
            //printf("%d |", matrix_b[i][j]);
        }
        //printf("\n");
    }

   printf("\n");

 // print matrix_c
    printf("matrix_c\n");
    for(int i=0;i<MATRIX_SIZE;i++){
        for(int j=0;j<MATRIX_SIZE;j++){ ;
            printf("%f |", matrix_c[i][j]);
        }
        printf("\n");
    }
*/
    printf("\n");

}

int main() {

    // init matrix a,b,c
    int_gemm();

    struct timespec requestStart, requestEnd;

    clock_gettime(CLOCK_REALTIME, &requestStart); // start

    core_bmm(0);

    clock_gettime(CLOCK_REALTIME, &requestEnd); // end


    // Calculate time it took
    double accum = ( requestEnd.tv_sec - requestStart.tv_sec )
                   + ( requestEnd.tv_nsec - requestStart.tv_nsec )
                     / BILLION;
    printf( "Time taken: %lf\n", accum );

    // print matrix a,b,c
    print_gemm();



}
