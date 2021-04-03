#define THLS_SYNTHESIS
#define DTHLS_FW_UINT_ON_AP_UINT 1

#include <stdio.h>
#include "ap_int.h"

#include <math.h>
#include <iostream>
#include <array>

#include <thls/tops/fw_uint.hpp>
#include <thls/tops/fp_flopoco.hpp>
#include <thls/tops/fp_flopoco_mul_v1.hpp>
#include <thls/tops/fp_flopoco_add_dual_v1.hpp>
#include <thls/tops/fp_convert.hpp>

/**************************************************************************************************************/

#define NUMOFCORE_OFFSET 0
#define MATRIX_SIZE_OFFSET 1
#define MATRIX_LOCALINE_START_OFFSET 1

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
	// local data
	fp_t* local_mem_flopoco;
	fp_t local_a_row[64];
#pragma HLS bind_storage variable=local_a_row impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=local_a_row cyclic factor=16 dim=1
	fp_t local_b[64][16];
#pragma HLS bind_storage variable=local_b impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=local_b cyclic factor=16 dim=2
	fp_t local_c_row[64];
#pragma HLS array_partition variable=local_c_row cyclic factor=16 dim=1


	// read first 16 float
	float* local_mem_float= getLocalLineFloat(mem,0);


	// set numofcore - matrix_size
	float numofcore = local_mem_float[NUMOFCORE_OFFSET];
	int matrix_size =  local_mem_float[MATRIX_SIZE_OFFSET];
	//core_number=0;
	 //set zero
	fp_t zero_var;
	zero_var.bits.operator ==(0);

	// set start - end
	int start = ((matrix_size/numofcore) * core_number);
	int end = ((matrix_size/numofcore) * (core_number+1));

	// set matrix offset
	int matrix_A_offset = MATRIX_LOCALINE_START_OFFSET;
	int matrix_B_offset = matrix_A_offset + (matrix_size * matrix_size) / 16;
    int matrix_C_offset = matrix_B_offset + (matrix_size * matrix_size) / 16;



   loop_first: for (int i = start; i < end; i++) {
#pragma HLS PIPELINE II = 1
    	// load local A row
    	loop_a:for(int j=0;j<matrix_size/16;j++){
    			local_mem_flopoco = getLocalLineFlopoco(mem,matrix_A_offset + i*matrix_size/16 + j);
    				for(int k=0;k<16;k++){
    					local_a_row[j*16 + k] = local_mem_flopoco[k];

   				}

    	}

    	// mul - write into local C row
    	loop_mul:for (int j = 0; j < matrix_size/16; j++) {
#pragma HLS PIPELINE II = 1
    		 //load local B column(16)
    	     for(int m=0; m<matrix_size;m++){
    	    	 local_mem_flopoco=getLocalLineFlopoco(mem,matrix_B_offset  + m*matrix_size/16 + j );
    	    	 for(int n=0; n<16;n++){
    	    		 local_b[m][n]= local_mem_flopoco[n];

    	    	 }

    	     }
    	     for(int z=0;z<16;z++){
    	    	 fp_t temp = zero_var;///change
    	    	 for (int k = 0; k < matrix_size; k++) {
    	    	     temp= thls::add_dual<E,F>(temp, thls::mul_v1<E,F>(local_a_row[k],local_b[k][z],0),0);
    	    	  }
    	    	  local_c_row[j*16 + z] = temp;
    	     }

    	}

    	// write local C row
		loop_c:for(int j=0;j<matrix_size/16;j++){
#pragma HLS PIPELINE II = 1
			writeLocalLineFlopoco(mem,&local_c_row[16*j],matrix_C_offset + i*matrix_size/16 + j);

		}

    }


}
