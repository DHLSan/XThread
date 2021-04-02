#define THLS_SYNTHESIS
#define DTHLS_FW_UINT_ON_AP_UINT 1

#include <stdio.h>
#include <ap_int.h>

#include <math.h>
#include <iostream>
#include <array>

#include <thls/tops/fw_uint.hpp>
#include <thls/tops/fp_flopoco_div_v2.hpp>
#include <thls/tops/fp_flopoco_mul_v1.hpp>
#include <thls/tops/fp_flopoco_neg_v1.hpp>
#include <thls/tops/fp_flopoco_add_dual_v1.hpp>


#include "xthread.h"

/****************  MEMORY OFFSETS ***********************/

#define NUMOFCORE_OFFSET 0
#define NUMOFVAR_OFFSET 1
#define MATRIX_LOCALLINE_OFFSET 1

xthread_barrier_t barrier_A; // holds all cores

/********************************************************/
using namespace thls;

const int E=8;
const int F=23;

using fp_t = thls::fp_flopoco<E,F>;

/********************************************************/

float* getLocalLineFloat(volatile ap_uint<512> *mem, int lineNumber){
#pragma HLS inline
	static float local_float_mem_16[16];

	ap_uint<512> local_line = mem[lineNumber];
	for(int i=0; i<16;i++){
#pragma HLS UNROLL FACTOR=16
		unsigned int itemp = local_line((i+1)*32-1,i*32);
		float ftemp = *(float*)(&itemp);
		local_float_mem_16[i] = ftemp;
	}

	return local_float_mem_16;
}


fp_t* getLocalLineFlopoco(volatile ap_uint<512> *mem, int lineNumber){
#pragma HLS inline

	static fp_t local_flopoco_mem_16[16];

	ap_uint<512> local_line = mem[lineNumber];;
	for(int i=0; i<16;i++){
#pragma HLS UNROLL FACTOR=16

		ap_uint<32> ap_ui_temp = local_line((i+1)*32-1,i*32);

	    fw_uint<2> flags = fw_uint<2>(0b01); // FP_NORMAL
	    fw_uint<1> sign = fw_uint<1>(ap_ui_temp.range(31,31).to_uint());
	    fw_uint<E> exp = fw_uint<E>(ap_ui_temp.range(30,23).to_uint());
	    fw_uint<F> frac = fw_uint<F>(ap_ui_temp.range(22,0).to_uint());

	    // flags + sign + exp + frac
	    fw_uint<2+1+E+F> bits = concat(flags,sign,exp,frac);

	    local_flopoco_mem_16[i] = thls::fp_flopoco<E,F>(bits);

	}

	return local_flopoco_mem_16;
}

void writeLocalLineFlopoco(volatile ap_uint<512> *mem, fp_t local_flopoco_mem[16], int lineNumber){
#pragma HLS inline

    static const double pos_zero = 0.0;
    static const double neg_zero = 1.0 / -INFINITY;

	ap_uint<512> local_line;

	for(int i=0;i<16;i++){
#pragma HLS UNROLL FACTOR=16

		// convert flopocco into ap_uint<32>

		if(local_flopoco_mem[i].is_zero().to_bool()){
			   local_line((i+1)*32-1,i*32) = local_flopoco_mem[i].is_negative().to_bool() ? neg_zero : pos_zero;
		}

		else {
			ap_uint<64> ap_ui_temp = local_flopoco_mem[i].bits.to_uint64();
			// get sign + exp + frac
			local_line((i+1)*32-1,i*32) = ap_ui_temp(31,0);
		}

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

	/////////////////////////////////////////


	xthread_barrier_init_core(&barrier_A); // init barrier_A

	// get first 16 float
	float* local_mem_float = getLocalLineFloat(mem,0);

	float numofcore = local_mem_float[NUMOFCORE_OFFSET];
	float numofvar_f = local_mem_float[NUMOFVAR_OFFSET];
	int numofvar_i =  numofvar_f;

	int pivot = 0; // start point of pivot is 0

	// pivot row - current row
	fp_t* local_mem_flopoco;

	fp_t pivotRow[1023 + 1];
#pragma HLS bind_storage variable=pivotRow impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=pivotRow cyclic factor=8 dim=1
	fp_t currentRow[1023 + 1];
#pragma HLS array_partition variable=currentRow block factor=2 dim=1

	while(pivot < numofvar_i){

		// load pivot row by pivot number - start by pivot th element
		for(int j=pivot/16;j<(numofvar_i+1)/16;j++){
#pragma HLS PIPELINE II = 1
			local_mem_flopoco = getLocalLineFlopoco(mem,MATRIX_LOCALLINE_OFFSET + pivot * (numofvar_i+1)/16 + j);
			for(int k=0;k<16;k++){
				pivotRow[j*16 + k] = local_mem_flopoco[k];
			}
		}

		for(int current=(numofvar_f/numofcore)*core_number; current<=(numofvar_f/numofcore)*(core_number+1)-1; current++){

			 // jump over the row if it is pivot
			if(current==pivot) continue;

			// load current row by pivot number - start by pivot'th element
			for(int j=pivot/16;j<(numofvar_i+1)/16;j++){
#pragma HLS PIPELINE II = 1
				local_mem_flopoco = getLocalLineFlopoco(mem,MATRIX_LOCALLINE_OFFSET + current * (numofvar_i+1)/16 + j);
				for(int k=0;k<16;k++){
					currentRow[j*16 + k] = local_mem_flopoco[k];
				}
			}

			fp_t multfact = thls::div_v2<E,F>(currentRow[pivot],pivotRow[pivot],0);
			for(int i=pivot;i<=numofvar_i;i++){
#pragma HLS PIPELINE II = 1
				currentRow[i] = thls::add_dual<E,F>(currentRow[i],thls::neg_v1<E,F>(thls::mul_v1<E,F>(multfact,pivotRow[i],0),0),0);
			}

			// write currentrow starting by pivot'th element
			for(int j=pivot/16;j<(numofvar_i+1)/16;j++){
#pragma HLS PIPELINE II = 1
				writeLocalLineFlopoco(mem,&currentRow[j*16],MATRIX_LOCALLINE_OFFSET + current * (numofvar_i+1)/16 + j);
			}

		}

		pivot = pivot + 1;

		xthread_barrier_wait(control_unit,core_number,ack,&barrier_A,(float*)mem);

	}

}
