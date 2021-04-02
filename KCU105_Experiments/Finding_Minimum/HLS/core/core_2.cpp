#include <stdio.h>
#include "xthread.h"
#include "ap_int.h"

/**************************************************************************************************************/

#define MINIMUM_VALUE_OFFSET 0
#define PARTIAL_LIST_SIZE_OFFSET 1
#define LIST_LOCALLINE_OFFSET 1


xthread_mutex_t minimum_value_lock;

float* getLocalLine(volatile ap_uint<512> *mem, int lineNumber){
#pragma HLS inline
	static float local_float_mem[16];
	ap_uint<512> local_line = mem[lineNumber];

	for(int i=0; i<16;i++){
#pragma HLS UNROLL FACTOR=16
		unsigned int itemp = local_line((i+1)*32-1,i*32);
		float ftemp = *(float*)(&itemp);
		local_float_mem[i] = ftemp;
	}

	return local_float_mem;
}

void writeLocalLine(volatile ap_uint<512> *mem, float local_float_mem[16], int lineNumber){
#pragma HLS inline
	ap_uint<512> local_line;
	for(int i=0;i<16;i++){
#pragma HLS UNROLL FACTOR=16
		float ftemp = local_float_mem[i];
		unsigned int itemp = *(unsigned int*)(&ftemp);
		local_line((i+1)*32-1,i*32) = itemp;
	}

	mem[lineNumber] = local_line;

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

	// init mutex
	xthread_mutex_init_core(&minimum_value_lock);

	int myId = core_number + 1;
	int counter;
	float next_element;
	float my_min = 0xFFFFFFFF; // local
	float* local_mem = getLocalLine(mem,0);


	int lower = ((myId - 1) * (int)local_mem[PARTIAL_LIST_SIZE_OFFSET])/16;
	int upper = ((myId * (int)local_mem[PARTIAL_LIST_SIZE_OFFSET]) - 1)/16;

    for (counter = lower; counter <= upper; counter++){
#pragma HLS PIPELINE
		local_mem = getLocalLine(mem,counter+LIST_LOCALLINE_OFFSET);
		for(int i=0; i<16;i++){
			next_element = local_mem[i];
			if (next_element < my_min)
				my_min = next_element;
		}

    }
    local_mem = getLocalLine(mem,0);
    // Consider that global minimum will never be bigger than it is.
    if(my_min > local_mem[MINIMUM_VALUE_OFFSET]) return;

    /* lock the mutex associated with minimum_value and update the variable as required */
    xthread_mutex_lock(control_unit,core_number,ack,&minimum_value_lock,(float*)mem);
    	local_mem = getLocalLine(mem,0);
        if (my_min < local_mem[MINIMUM_VALUE_OFFSET]){
        	local_mem[MINIMUM_VALUE_OFFSET] = my_min;
			writeLocalLine(mem,local_mem,0);
		}


    xthread_mutex_unlock(control_unit,core_number,ack,&minimum_value_lock,(float*)mem);
    /* and unlock the mutex */

}
