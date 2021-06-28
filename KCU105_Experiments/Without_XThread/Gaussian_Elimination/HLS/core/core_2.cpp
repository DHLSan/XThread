#include <stdio.h>
#include "ap_int.h"

/**************************************************************************************************************/

/****************  MEMORY OFFSETS ***********************/

#define NUMOFCORE_OFFSET 0
#define NUMOFVAR_OFFSET 1
#define MATRIX_LOCALLINE_OFFSET 1

/****************  ***********************/

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

	// get first 16 float

	float* local_mem = getLocalLine(mem,0);

	float numofcore =local_mem[NUMOFCORE_OFFSET];
	float numofvar_f =local_mem[NUMOFVAR_OFFSET];
	int numofvar_i = numofvar_f;

	//core_number=0;
	int pivot = 0; // start point of pivot is 0
	// pivot row - current row
	float pivotRow[1023 + 1];
#pragma HLS bind_storage variable=pivotRow impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=pivotRow cyclic factor=8 dim=1
	float currentRow[1023 + 1];
//#pragma HLS bind_storage variable=currentRow impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=currentRow block factor=2 dim=1


	while_loop: while(pivot < numofvar_i){
		// load pivot row by pivot number - start by pivot th element
		load_pivot_1: for(int j=pivot/16;j<(numofvar_i+1)/16;j++){
#pragma HLS PIPELINE II = 1
			local_mem = getLocalLine(mem,MATRIX_LOCALLINE_OFFSET + pivot * (numofvar_i+1)/16 + j);
			load_pivot_2: for(int k=0;k<16;k++){
				pivotRow[j*16 + k] = local_mem[k];
			}
		}


		 main_for: for(int current=(numofvar_f/numofcore)*core_number; current<=(numofvar_f/numofcore)*(core_number+1)-1; current++){
			 // jump over the row if it is pivot
			if(current==pivot) continue;

			// load current row by pivot number - start by pivot'th element
			load_current_1: for(int j=pivot/16;j<(numofvar_i+1)/16;j++){
#pragma HLS PIPELINE II = 1
				local_mem = getLocalLine(mem,MATRIX_LOCALLINE_OFFSET + current * (numofvar_i+1)/16 + j);
				load_current_2: for(int k=0;k<16;k++){
					currentRow[j*16 + k] = local_mem[k];
				}
			}

			float multfact = currentRow[pivot] / pivotRow[pivot];
//#pragma HLS BIND_OP variable=multfact op=fdiv impl=fabric latency=2
			mul_for: for(int i=pivot;i<=numofvar_i;i++){
#pragma HLS PIPELINE II = 1
				currentRow[i] = (currentRow[i]) - (multfact * pivotRow[i]);
			}

			// write currentrow starting by pivot'th element
			write_current: for(int j=pivot/16;j<(numofvar_i+1)/16;j++){
#pragma HLS PIPELINE II = 1
				writeLocalLine(mem,&currentRow[j*16],MATRIX_LOCALLINE_OFFSET + current * (numofvar_i+1)/16 + j);
			}

		}

		pivot = pivot + 1;

	}

}
