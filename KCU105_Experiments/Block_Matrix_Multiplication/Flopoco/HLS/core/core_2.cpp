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


#define BLOCK_SIZE 16
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

	// read first 16 float
	float* local_mem_float= getLocalLineFloat(mem,0);

    /****************************************/

    // set numofcore - matrix_size
    float numofcore = local_mem_float[NUMOFCORE_OFFSET];
    int matrix_size =  local_mem_float[MATRIX_SIZE_OFFSET];

	// set matrix offset
	int matrix_A_offset = MATRIX_LOCALINE_START_OFFSET;
	int matrix_B_offset = matrix_A_offset + (matrix_size * matrix_size) / 16;
    int matrix_C_offset = matrix_B_offset + (matrix_size * matrix_size) / 16;

    fp_t* local_mem_flopoco;

    fp_t local_a[BLOCK_SIZE][BLOCK_SIZE];
#pragma HLS array_partition variable=local_a cyclic factor=4  dim=2
    fp_t local_b[BLOCK_SIZE][BLOCK_SIZE];
#pragma HLS array_partition variable=local_b cyclic factor=4 dim=1
   fp_t local_c[BLOCK_SIZE][BLOCK_SIZE];
#pragma HLS bind_storage variable=local_c impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=local_c cyclic factor=2 dim=2


   //set zero
   fp_t zero_var;
   zero_var.bits.operator ==(0);

    int start_blocked_row = (matrix_size / numofcore) * core_number / BLOCK_SIZE;
    int end_blocked_row = (matrix_size / numofcore) * (core_number + 1) / BLOCK_SIZE;

    // i indicates blocked_row
    for (int i = start_blocked_row; i < end_blocked_row; i++) {

        // j indicates blocked_column
        for (int j = 0; j < matrix_size / BLOCK_SIZE; j++) {

            // reset local_c
       c_loop:     for (int x = 0; x < BLOCK_SIZE; x++) {
                for (int y = 0; y < BLOCK_SIZE; y++) {
#pragma HLS PIPELINE II = 1
                    local_c[x][y]=zero_var;
                }
            }

            // k indicates kth block on given i,j pair
            for(int k = 0; k < matrix_size / BLOCK_SIZE; k++){

            	// load local_a
                for(int x=0;x < BLOCK_SIZE; x++){
#pragma HLS PIPELINE II = 1
                	local_mem_flopoco = getLocalLineFlopoco(mem,matrix_A_offset + i*BLOCK_SIZE*matrix_size/16 + k*BLOCK_SIZE/16 + x*matrix_size/16);
        	        for(int y=0; y < 16; y++){
        	        	local_a[x][y] = local_mem_flopoco[y];
        	        }
                }

                // load local_b
           b_loop:    for(int x=0;x < BLOCK_SIZE; x++){
#pragma HLS PIPELINE II = 1
                	local_mem_flopoco = getLocalLineFlopoco(mem,matrix_B_offset + k*BLOCK_SIZE*matrix_size/16 + j*BLOCK_SIZE/16 + x*matrix_size/16);
        	        for(int y=0; y < 16; y++){
        	        	local_b[x][y] = local_mem_flopoco[y];
        	        }
                }

                // mul inner matrix
               x_loop: for (int x = 0; x < BLOCK_SIZE; x++) {
            	   y_loop:     for (int y = 0; y < BLOCK_SIZE; y++) {
#pragma HLS PIPELINE II = 1
                        fp_t temp = zero_var;
                        z_loop:          for (int z = 0; z < BLOCK_SIZE; z++) {
                            temp = thls::add_dual<E,F>(temp, thls::mul_v1<E,F>(local_a[x][z],local_b[z][y],0),0);
                        }
                        local_c[x][y] =thls::add_dual<E,F>( local_c[x][y], temp,0);
                    }
                }

            }

            // write local_c
     f_loop:       for(int x=0; x < BLOCK_SIZE; x++){
#pragma HLS PIPELINE II = 1
            	writeLocalLineFlopoco(mem,&local_c[x][0],matrix_C_offset + i*BLOCK_SIZE*matrix_size/16 + j*BLOCK_SIZE/16 + x*matrix_size/16);
            }

        }

    }


}

