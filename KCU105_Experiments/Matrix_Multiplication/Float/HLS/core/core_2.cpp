#include <stdio.h>
#include "ap_int.h"

/**************************************************************************************************************/

#define NUMOFCORE_OFFSET 0
#define MATRIX_SIZE_OFFSET 1
#define MATRIX_LOCALINE_START_OFFSET 1

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
	// local data
	float local_a_row[128];
#pragma HLS bind_storage variable=local_a_row impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=local_a_row cyclic factor=16 dim=1
	float local_b[128][16];
#pragma HLS bind_storage variable=local_b impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=local_b cyclic factor=16 dim=2
//#pragma HLS array_partition variable=local_b block factor=2 dim=1
	float local_c_row[128];
//#pragma HLS bind_storage variable=local_c_row impl=BRAM type=RAM_1WNR
#pragma HLS array_partition variable=local_c_row cyclic factor=16 dim=1

	// read first 16 float
	float* local_mem = getLocalLine(mem,0);

	// set numofcore - matrix_size
	float numofcore = local_mem[NUMOFCORE_OFFSET];
	int matrix_size =  local_mem[MATRIX_SIZE_OFFSET];
	//core_number=0;

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
    				local_mem = getLocalLine(mem,matrix_A_offset + i*matrix_size/16 + j);
    				for(int k=0;k<16;k++){
    					local_a_row[j*16 + k] = local_mem[k];

   				}

    	}

    	// mul - write into local C row
    	loop_mul:for (int j = 0; j < matrix_size/16; j++) {
#pragma HLS PIPELINE II = 1
    		 //load local B column(16)
    	     for(int m=0; m<matrix_size;m++){
    	    	 local_mem=getLocalLine(mem,matrix_B_offset  + m*matrix_size/16 + j );
    	    	 for(int n=0; n<16;n++){
    	    		 local_b[m][n]= local_mem[n];

    	    	 }

    	     }
    	     for(int z=0;z<16;z++){
    	    	 float temp = 0;///change
    	    	 for (int k = 0; k < matrix_size; k++) {
    	    	     temp += local_a_row[k] * local_b[k][z];
    	    	  }
    	    	  local_c_row[j*16 + z] = temp;
    	     }

    	}

    	// write local C row
		loop_c:for(int j=0;j<matrix_size/16;j++){
#pragma HLS PIPELINE II = 1
			writeLocalLine(mem,&local_c_row[16*j],matrix_C_offset + i*matrix_size/16 + j);

		}

    }


}
