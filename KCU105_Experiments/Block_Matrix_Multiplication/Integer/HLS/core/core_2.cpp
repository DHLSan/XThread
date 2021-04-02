#include <stdio.h>
#include "ap_int.h"

/**************************************************************************************************************/

#define NUMOFCORE_OFFSET 0
#define MATRIX_SIZE_OFFSET 1
#define MATRIX_LOCALINE_START_OFFSET 1

#define BLOCK_SIZE 16

int* getLocalLine(volatile ap_uint<512> *mem, int lineNumber){
#pragma HLS inline
	static int local_int_mem[16];
	ap_uint<512> local_line = mem[lineNumber];

	for(int i=0; i<16;i++){
#pragma HLS UNROLL FACTOR=16
		unsigned int itemp = local_line((i+1)*32-1,i*32);
		int ftemp = *(int*)(&itemp);
		local_int_mem[i] = ftemp;
	}

	return local_int_mem;
}

void writeLocalLine(volatile ap_uint<512> *mem, int local_int_mem[16], int lineNumber){
#pragma HLS inline
	ap_uint<512> local_line;
	for(int i=0;i<16;i++){
#pragma HLS UNROLL FACTOR=16
		int ftemp = local_int_mem[i];
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

	// read first 16 float
	int* local_mem = getLocalLine(mem,0);

    /****************************************/

    // set numofcore - matrix_size
    float numofcore = local_mem[NUMOFCORE_OFFSET];
    int matrix_size =  local_mem[MATRIX_SIZE_OFFSET];

	// set matrix offset
	int matrix_A_offset = MATRIX_LOCALINE_START_OFFSET;
	int matrix_B_offset = matrix_A_offset + (matrix_size * matrix_size) / 16;
    int matrix_C_offset = matrix_B_offset + (matrix_size * matrix_size) / 16;

    int local_a[BLOCK_SIZE][BLOCK_SIZE];
#pragma HLS bind_storage variable=local_a impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=local_a cyclic factor=16 dim=2
    int local_b[BLOCK_SIZE][BLOCK_SIZE];
#pragma HLS bind_storage variable=local_b impl=LUTRAM type=RAM_1WNR
#pragma HLS array_partition variable=local_b cyclic factor=16 dim=1
    int local_c[BLOCK_SIZE][BLOCK_SIZE];
//#pragma HLS bind_storage variable=local_c impl=BRAM type=RAM_T2P
//#pragma HLS array_partition variable=local_c cyclic factor=2 dim=2

    int start_blocked_row = (matrix_size / numofcore) * core_number / BLOCK_SIZE;
    int end_blocked_row = (matrix_size / numofcore) * (core_number + 1) / BLOCK_SIZE;


    // i indicates blocked_row
    for (int i = start_blocked_row; i < end_blocked_row; i++) {

        // j indicates blocked_column
        for (int j = 0; j < matrix_size / BLOCK_SIZE; j++) {

            // reset local_c
            for (int x = 0; x < BLOCK_SIZE; x++) {
#pragma HLS UNROLL FACTOR = 16
                for (int y = 0; y < BLOCK_SIZE; y++) {
                    local_c[x][y] = 0;
                }
            }

            // k indicates kth block on given i,j pair
            for(int k = 0; k < matrix_size / BLOCK_SIZE; k++){

            	// load local_a
                for(int x=0;x < BLOCK_SIZE; x++){
#pragma HLS PIPELINE II = 1
                	local_mem = getLocalLine(mem,matrix_A_offset + i*BLOCK_SIZE*matrix_size/16 + k*BLOCK_SIZE/16 + x*matrix_size/16);
        	        for(int y=0; y < 16; y++){
        	        	local_a[x][y] = local_mem[y];
        	        }
                }

                // load local_c
                for(int x=0;x < BLOCK_SIZE; x++){
#pragma HLS PIPELINE II = 1
                	local_mem = getLocalLine(mem,matrix_B_offset + k*BLOCK_SIZE*matrix_size/16 + j*BLOCK_SIZE/16 + x*matrix_size/16);
        	        for(int y=0; y < 16; y++){
        	        	local_b[x][y] = local_mem[y];
        	        }
                }

                // mul inner matrix
               x_loop: for (int x = 0; x < BLOCK_SIZE; x++) {
            	   y_loop:     for (int y = 0; y < BLOCK_SIZE; y++) {
#pragma HLS PIPELINE II = 1
                        int temp = 0;
                        z_loop:          for (int z = 0; z < BLOCK_SIZE; z++) {
                            temp += local_a[x][z] * local_b[z][y];
                        }
                        local_c[x][y] = local_c[x][y] + temp;
                    }
                }

            }

            // write local_c
            for(int x=0; x < BLOCK_SIZE; x++){
#pragma HLS PIPELINE II = 1
            	writeLocalLine(mem,&local_c[x][0],matrix_C_offset + i*BLOCK_SIZE*matrix_size/16 + j*BLOCK_SIZE/16 + x*matrix_size/16);
            }

        }

    }






}
