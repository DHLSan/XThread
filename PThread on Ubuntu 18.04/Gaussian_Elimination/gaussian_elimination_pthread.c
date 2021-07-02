#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define NUMOFCORE 12
#define NUMOFVAR 1023
#define BILLION  1E9;
pthread_barrier_t barrier_A; // holds all cores

/* Memory
 * ----------------------------------------------------------
 */

float equations[NUMOFVAR][NUMOFVAR+1];

int pivot = 0; // init value

/* Unit
 * ----------------------------------------------------------
 */

void loadequations(){

    int prime = 1;

    for(int i=0;i<NUMOFVAR;i++){
        for(int j=0;j<NUMOFVAR+1;j++){

            equations[i][j] = prime+j+i*i+2500;

            /*for(int k=prime+1;;k++){

                for(int g=2;g<k;g++){
                    if( k % g == 0) break;
                    if( g == k-1 ) prime = k;
                }
                if(prime==k) break;

            }*/

        }
    }

}

void printmatrix(){

    printf("\n");
    for(int i=0;i<NUMOFVAR;i++){
        for(int j=0;j<NUMOFVAR+1;j++){
            printf("| %lf |",equations[i][j]);
        }
        printf("\n");
    }
}

void set(){
    pivot++; // take one step, next pivot
    //printf("\n");
}

void core(int coreNo){

    /*
     * Algorithm: Gaussian Elimination and Back-Substitution Without Interchange
     */

    float numofcore = NUMOFCORE;
    float numofvar = NUMOFVAR;

    int current; // current row

    while(pivot < numofvar){

        for(int k=(numofvar/numofcore)*coreNo;k<=(numofvar/numofcore)*(coreNo+1)-1;k++){

            current = k;

            if(k==pivot) continue; // pass the pivot

            //printf("\ncore %d -> has -> row: %d in pivot: %d",coreNo,current,pivot);

            // multiplication factor
            float multfact =  equations[current][pivot] / equations[pivot][pivot];
            for(int i=pivot; i<= numofvar; i++){
                equations[current][i] = (equations[current][i]) - (multfact * equations[pivot][i]);
            }

        }

        pthread_barrier_wait(&barrier_A);
        if(coreNo==0) set();
        pthread_barrier_wait(&barrier_A);

    }

}

void *runner(void *params){
    int coreNo = (int) params;

    core(coreNo);

    return NULL;
}

int main(){
   struct timespec requestStart, requestEnd;

    pthread_barrier_init(&barrier_A, NULL, NUMOFCORE);

    loadequations();
    //printmatrix();


    pthread_t tid[NUMOFCORE];
    clock_gettime(CLOCK_REALTIME, &requestStart);
    /** Create */
    for (int i = 0; i < NUMOFCORE; ++i) {
        pthread_create(&tid[i], NULL, runner, i);
    }

    //printf("\n");

    /** Join */
    for (int i = 0; i < NUMOFCORE; ++i) {
        pthread_join(tid[i], NULL);
    }
    clock_gettime(CLOCK_REALTIME, &requestEnd);
    double accum = ( requestEnd.tv_sec - requestStart.tv_sec )
       + ( requestEnd.tv_nsec - requestStart.tv_nsec )
       / BILLION;
     printf( "Time taken: %lf\n", accum );


    /** Print */
    //printmatrix();

    return 0;
}
