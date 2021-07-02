/*************************************************************************

 Objective  :  Finds the Minimum Value in the Integer List. 

 Input      :  Number of Threads,
         
               Integer List Size.

 Output     :  Minimum value in the Integer List

*************************************************************************/

#include<pthread.h>
#include<stdio.h>
#include<sys/time.h>
#include<stdlib.h>
#include <math.h>
#define THREADS 1

#define NumElements 100000000
#define BILLION  1E9;
void *find_min(void *) ;
void init_finding_min();

// mutex accessible by threads
pthread_mutex_t minimum_value_lock;

long int partial_list_size;
float minimum_value;

float list_1[NumElements];
int NumThreads;

int main (int argc,char * argv[]) {

    pthread_t       *threads;
    pthread_attr_t  pta;
    double          time_start, time_end;
    struct          timeval tv;
    struct          timezone tz;
    double          MemoryUsed = 0.0;
    int counter;
    //printf("\n\t\t---------------------------------------------------------------------------");
   // printf("\n\t\t Centre for Development of Advanced Computing (C-DAC):  December-2006");
   // printf("\n\t\t C-DAC Multi Core Benchmark Suite 1.0");
   // printf("\n\t\t Email : hpcfte@cdac.in");
    //printf("\n\t\t---------------------------------------------------------------------------");
   // printf("\n\t\t Objective : Sorting Single Dimension Array (Integer Operations)\n ");
   // printf("\n\t\t Performance of Sorting a Minimum value in a large Single Dimension Array ");
    //printf("\n\t\t on Multi Socket Multi Core Processor using 1/2/4/8 threads \n");
    //printf("\n\t\t Input Parameters :");
   // printf("\n\t\t..........................................................................\n");

    NumThreads = THREADS;
    printf("\n\t\t Array Size  :  %d",NumElements);
    printf("\n\t\t Threads     :  %d",NumThreads);
    //printf("\n");

    partial_list_size = NumElements / NumThreads;

    //list = (long int *)malloc(sizeof(long int) * NumElements);
    MemoryUsed += ( NumElements * sizeof(long int));



    threads = (pthread_t *)malloc(sizeof(pthread_t)*NumThreads);

    // pthread attr
    pthread_mutex_init(&minimum_value_lock, NULL);
    pthread_attr_init(&pta);
    pthread_attr_setscope(&pta,PTHREAD_SCOPE_SYSTEM);

    // load list
    init_finding_min();

    gettimeofday(&tv, &tz);
    time_start = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
    // create threads
    for(counter = 0 ; counter < NumThreads ; counter++)
        pthread_create(&threads[counter],&pta,(void *(*) (void *)) find_min,(void *) (counter+1));

    // join threads
    for(counter = 0 ; counter < NumThreads ; counter++)
        pthread_join(threads[counter],NULL);

    pthread_attr_destroy(&pta);

    gettimeofday(&tv, &tz);
    time_end = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;

    printf("\n\t\t Minimum Value found in the Float list     :  %f",minimum_value);
    //printf("\n\t\t Memory Utilised                             :  %lf MB",(MemoryUsed / (1024*1024)));
    printf("\n\t\t Time Taken in Seconds  (T)                  :  %lf Seconds",( time_end - time_start));
    printf("\n\t\t   ( T represents the Time taken to  Minimum Value )\n");
    printf("\n\t\t..........................................................................\n");

    free(threads);

    return 0;

}

void *find_min(void * myid ) {

    float  my_min; // local
    long int counter; // local

    int myId = (int)myid; // local

    my_min = list_1[(myId-1)*partial_list_size]; // local

    for (counter = ((myId - 1) * partial_list_size); counter <= ((myId * partial_list_size) - 1); counter++){
        if (list_1[counter] < my_min)
            my_min = list_1[counter];
    }

    /* lock the mutex associated with minimum_value and update the variable as
    required */

    if (my_min > minimum_value) pthread_exit(NULL);

    pthread_mutex_lock(&minimum_value_lock) ;
    if (my_min < minimum_value)
        minimum_value = my_min;

    /* and unlock the mutex */
    pthread_mutex_unlock(&minimum_value_lock) ;

}

void init_finding_min(){

    // set min value
    minimum_value = 0xFFFFFF;

    // load list
    for(int i=0;i<NumElements;i++){
        list_1[i] = i + 95;
    }
	list_1[20000]=90;

}
