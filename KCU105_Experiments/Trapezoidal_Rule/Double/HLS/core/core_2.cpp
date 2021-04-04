#include <stdio.h>
#include "xthread.h"
#include "ap_int.h"

/**************************************************************************************************************/


/****************  MEMORY OFFSETS ***********************/

#define NUMOFCORE_OFFSET 0
#define GLOBAL_MUTEX_SUM_OFFSET 1
#define A_OFFSET 2
#define B_OFFSET 3
#define N_OFFSET 4
#define H_OFFSET 5

/********************************************************/

xthread_mutex_t mutex;

/********************************************************/

double* getLocalLine(volatile ap_uint<512> *mem, int lineNumber){
#pragma HLS inline
	static double local_double_mem_8[8];

	ap_uint<512> local_line = mem[lineNumber];
	for(int i=0; i<8;i++){
#pragma HLS UNROLL FACTOR=8
		unsigned long long int itemp = local_line((i+1)*64-1,i*64);
		double ftemp = *(double*)(&itemp);
		local_double_mem_8[i] = ftemp;
	}

	return local_double_mem_8;
}

void writeLocalLine(volatile ap_uint<512> *mem, double local_double_mem[8], int lineNumber){
#pragma HLS inline
	ap_uint<512> local_line;
	for(int i=0;i<8;i++){
#pragma HLS UNROLL FACTOR=8
		double ftemp = local_double_mem[i];
		unsigned long long int itemp = *(unsigned long long int*)(&ftemp);
		local_line((i+1)*64-1,i*64) = itemp;
	}

	mem[lineNumber] = local_line;

}

// block_low and block_high provided by Dr. Karlsson for distributing work to threads
long long block_low(int core_number, int num_of_core, long long n){
    return (core_number)*(n)/(num_of_core);
}

long long block_high(int core_number, int num_of_core, long long n){
    return block_low(core_number+1,num_of_core,n)-1;
}

// This is the function that is called in the serial and parallel trapezoidal functions.
double f(double x) {
    double return_val = x*x;
    return return_val;
}

void core_2( /* Synchronization */ int *control_unit, int core_number, volatile int *ack,
		     /* Memory */ volatile ap_uint<512> *mem){

#pragma HLS INTERFACE s_axilite port=return bundle=BUS_SLAVE
#pragma HLS INTERFACE s_axilite port=core_number bundle=BUS_SLAVE
#pragma HLS INTERFACE s_axilite port=ack bundle=BUS_SLAVE

#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=BUS_MEM
#pragma HLS INTERFACE s_axilite port=mem bundle=BUS_SLAVE

#pragma HLS INTERFACE m_axi port=control_unit offset=slave bundle=BUS_CU
#pragma HLS INTERFACE s_axilite port=control_unit bundle=BUS_SLAVE

	/****************************************/

	double *local_mem = getLocalLine(mem,0);

	double numofcore = local_mem[NUMOFCORE_OFFSET];
	double a = local_mem[A_OFFSET];
	double b = local_mem[B_OFFSET];
	double n = local_mem[N_OFFSET];
	double h = local_mem[H_OFFSET];

    long thread_rank = (long)core_number;
    double local_sum = 0.0;
    long long i;

    // allocate a chunk of work to the thread
    long long my_first_i = block_low(thread_rank, (int) numofcore, (long long) n);
    long long my_last_i = block_high(thread_rank, (int) numofcore, (long long) n);

    // let thread with rank 1 add (f(a)+f(b))/2.0 to it's sum. This is only
    // done once for trapezoidal rule calculation & thread that does it is
    // should be 1st or 2nd in case code is only run on 2 cores.
    if(core_number == 0) {
        local_sum += (f(a)+f(b))/2.0;
    }

    // Karlsson's trapezoid code
    for( i= my_first_i; i <= my_last_i; i++) {
#pragma HLS PIPELINE II=1
        local_sum += f(a + (i * h));
    }
    local_sum = local_sum * h;

    // update the global sum
    xthread_mutex_lock(control_unit,core_number,ack,&mutex,(float*)mem);

    local_mem = getLocalLine(mem,0);

    local_mem[GLOBAL_MUTEX_SUM_OFFSET] = local_mem[GLOBAL_MUTEX_SUM_OFFSET] + local_sum;

    writeLocalLine(mem,local_mem,0);

    xthread_mutex_unlock(control_unit,core_number,ack,&mutex,(float*)mem);

}

