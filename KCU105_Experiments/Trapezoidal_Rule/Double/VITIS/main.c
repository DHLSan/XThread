//send data over UART
#include "xil_printf.h"

//information about AXI peripherals
#include "xparameters.h"
#include "timerHelper.h"
// Timer
#include "xtmrctr.h"

#include <math.h>

#include "xthread.h"

int *control_unit_1_0 = (int *) 0x44A10000;
int *core_1_0 = (int *) 0x44A20000;
double *mem_float = (double *) 0x80000000; // write purpose

/*** Algorithm Specific ***/

#define NUMOFCORE 1

#define NUMOFCORE_OFFSET 0
#define GLOBAL_MUTEX_SUM_OFFSET 1
#define A_OFFSET 2
#define B_OFFSET 3
#define N_OFFSET 4
#define H_OFFSET 5


void create_trapezoidal(double *mem){

    mem[NUMOFCORE_OFFSET] = NUMOFCORE;
    mem[A_OFFSET] = 0;         // left endpoint
    mem[B_OFFSET] = 1024;         // right endpoint
    mem[N_OFFSET] = 2000 * (1024*1024); // number of trapezoids
    mem[H_OFFSET] = (mem[B_OFFSET] - mem[A_OFFSET]) / mem[N_OFFSET];;         // height of trapezoids

    xil_printf("\n\rcreated trapezoidal\n\r");
}
// block_low and block_high provided by Dr. Karlsson for distributing work to threads
long long block_low(int core_number, int num_of_core, long long n){
    return (core_number)*(n)/(num_of_core);
}

long long block_high(int core_number, int num_of_core, long long n){
    return block_low(core_number+1,num_of_core,n)-1;
}
double f(double x) {
    double return_val = x*x;
    return return_val;
}
void core(int core_number, double* mem){

    /****************************************/
    long thread_rank = (long)core_number;
    double local_sum = 0.0;
    long long i;

    // allocate a chunk of work to the thread
    long long my_first_i = block_low(thread_rank, (int) mem[NUMOFCORE_OFFSET], (long long) mem[N_OFFSET]);
    long long my_last_i = block_high(thread_rank, (int) mem[NUMOFCORE_OFFSET], (long long) mem[N_OFFSET]);

    //printf("\ncore %d | %lli to %lli",core_number,my_first_i,my_last_i);

    // let thread with rank 1 add (f(a)+f(b))/2.0 to it's sum. This is only
    // done once for trapezoidal rule calculation & thread that does it is
    // should be 1st or 2nd in case code is only run on 2 cores.
    if( core_number == 0) {
        local_sum += (f(mem[A_OFFSET])+f(mem[B_OFFSET]))/2.0;
    }

    // Karlsson's trapezoid code
    for( i= my_first_i; i <= my_last_i; i++) {
        local_sum += f(mem[A_OFFSET] + (i * mem[H_OFFSET]));
    }
    local_sum = local_sum * mem[H_OFFSET];

    // update the global sum

    mem[GLOBAL_MUTEX_SUM_OFFSET] += local_sum;


}

void printresult(double *mem){

	xil_printf("\rWith n = %ld trapezoids,\n", (long int) mem[N_OFFSET]);
	xil_printf("\r our estimate of the integral from %ld to %ld = %.15ld\n\n", (long int)mem[A_OFFSET], (long int)mem[B_OFFSET], (long int)mem[GLOBAL_MUTEX_SUM_OFFSET]);

}


void print_cores(){

	xil_printf("\n\rcu: %d - %X - %d - %d - %d - %X",control_unit_1_0[0],control_unit_1_0[4],control_unit_1_0[6],control_unit_1_0[8],control_unit_1_0[10],control_unit_1_0[12]);

	for(int i=0;i<NUMOFCORE;i++){
		int* c = (int*)(0x44A20000 + i*0x00010000);
		xil_printf("\n\rcore_%d: %d - %X - %d - %d - %X- %X- %X",i,c[0],c[4],c[6],c[8],c[10],c[11]);
	}

}

void setup(){
	// Give address of the first core to the controller, thus the controller knows where to start counting
	control_unit_1_0[12] = (int) core_1_0;
}

int main()
{
	xthread_barrier_init_main(control_unit_1_0,NUMOFCORE); // init barrier_0 - holds all cores
	xthread_t t_id[NUMOFCORE];
	setup();

	xil_printf("\nEntering the main\n\r");
	// prepare memory
	xil_printf("\nStarting\n\r");
	create_trapezoidal(mem_float);
	//printresult(mem_float);
	//print_cores();
	xil_printf("\nFinished\n\r");


	timer_helper_start();

	for(int i=0; i<NUMOFCORE; i++){
		xthread_create_main(&t_id[i],(int*)(0x44A20000 + i*0x00010000),(int)control_unit_1_0,(int)mem_float);
	}
	for(int i=0; i<NUMOFCORE; i++){
		xthread_join_main(&t_id[i]);
	}

	//core(0,mem_float);
	timer_helper_stop();
	//print_cores();

	printresult(mem_float);
	xil_printf("\n\rExiting the main\n\r");

	return 0;

}
