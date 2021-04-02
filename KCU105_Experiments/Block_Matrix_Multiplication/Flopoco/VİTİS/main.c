//send data over UART
#include "xil_printf.h"

//information about AXI peripherals
#include "xparameters.h"
#include "stdint.h"
// Timer
#include "xtmrctr.h"
#include <limits.h>
#include <math.h>
#include "timerHelper.h"

#include "xthread.h"

int *control_unit_1_0 = (int *) 0x44A10000;
int *core_1_0 = (int *) 0x44A20000;
float *mem_float = (float *) 0x80000000; // write purpose

/*** Algorithm Specific ***/

#define NUMOFCORE 1
#define MATRIX_SIZE 32

#define NUMOFCORE_OFFSET 0
#define MATRIX_SIZE_OFFSET 1
#define MATRIX_START_OFFSET 16
#define BLOCK_SIZE 16

#define MATRIX_A_OFFSET MATRIX_START_OFFSET
#define MATRIX_B_OFFSET MATRIX_A_OFFSET + MATRIX_SIZE * MATRIX_SIZE
#define MATRIX_C_OFFSET MATRIX_B_OFFSET + MATRIX_SIZE * MATRIX_SIZE

void create_gemm(){

	mem_float[NUMOFCORE_OFFSET] = NUMOFCORE;
	mem_float[MATRIX_SIZE_OFFSET] = MATRIX_SIZE;

	// create matrix_A
    for (int i = 0; i < MATRIX_SIZE*MATRIX_SIZE; i++) {
    	mem_float[MATRIX_A_OFFSET + i] = i;
    }

    // create matrix_B
    for (int i = 0; i < MATRIX_SIZE*MATRIX_SIZE; i++) {
    	mem_float[MATRIX_B_OFFSET + i] = i;
    }

	// create matrix_C
	for (int i = 0; i < MATRIX_SIZE*MATRIX_SIZE; i++) {
	    mem_float[MATRIX_C_OFFSET + i] = 0;
	}



}


void print_gemm(){

	xil_printf("\n");
	xil_printf("\rnumofcore: %d \n",(int) mem_float[NUMOFCORE_OFFSET]);
	xil_printf("\rmatrix_size: %d \n",(int) mem_float[MATRIX_SIZE_OFFSET]);
	xil_printf("\n");

	// print matrix_A
	xil_printf("matrix_A\n");
	for(int i=0;i<MATRIX_SIZE;i++){
		for(int j=0;j<MATRIX_SIZE;j++){
			float f = mem_float[MATRIX_A_OFFSET + MATRIX_SIZE * i + j];
			xil_printf("%d |", (int) f);
		}
		xil_printf("\n");
	}

	xil_printf("\n");

	// print matrix_B
	xil_printf("matrix_B\n");
	for(int i=0;i<MATRIX_SIZE;i++){
		for(int j=0;j<MATRIX_SIZE;j++){
			float f = mem_float[MATRIX_B_OFFSET + MATRIX_SIZE * i + j];
			xil_printf("%d |", (int) f);
		}
		xil_printf("\n");
	}

	xil_printf("\n\r");

	// print matrix_C
	xil_printf("matrix_C\n");
	for(int i=0;i<MATRIX_SIZE;i++){
		for(int j=0;j<MATRIX_SIZE;j++){
			int f = mem_float[MATRIX_C_OFFSET + MATRIX_SIZE * i + j];
			xil_printf("%d |", (int) f);
		}
		xil_printf("\n\r");
	}

	xil_printf("\n\r");

}
/******/

void print_cores(){

	for(int i=0;i<NUMOFCORE;i++){
		int* c = (int*)(0x44A20000 + i*0x00010000);
		xil_printf("\n\rcore_%d: %d - %X - %d - %d - %X",i,c[0],c[4],c[6],c[8],c[10]);
	}

}
void core_bmm(int core_number){

    /****************************************/

    float numofcore = NUMOFCORE;
    int matrix_size =  MATRIX_SIZE;

    int start_blocked_row = (matrix_size/numofcore)*core_number / BLOCK_SIZE;
    int end_blocked_row = (matrix_size/numofcore)*(core_number+1) / BLOCK_SIZE;

    //printf("\ncore: %d | start_block = %d | end_block = %d",core_number,start_blocked_row,end_blocked_row);

    // i indicates blocked_row
    for (int i = start_blocked_row; i < end_blocked_row; i++) {
        // j indicates blocked_column
        for (int j = 0; j < matrix_size / BLOCK_SIZE; j++) {
            // k indicates kth block on given i,j pair
            for(int k = 0; k < matrix_size / BLOCK_SIZE; k++){
                // mul inner matrix
                for (int x = 0; x < BLOCK_SIZE; x++) {
                    for (int y = 0; y < BLOCK_SIZE; y++) {
                        float temp = 0;
                        for (int z = 0; z < BLOCK_SIZE; z++) {
                            temp += mem_float[MATRIX_A_OFFSET+(i*BLOCK_SIZE + x)*matrix_size + k*BLOCK_SIZE + z] * mem_float[MATRIX_B_OFFSET+(k*BLOCK_SIZE + z)*matrix_size + j*BLOCK_SIZE + y];
                        }
                        mem_float[MATRIX_C_OFFSET + (i*BLOCK_SIZE + x)*matrix_size + j*BLOCK_SIZE + y] = mem_float[MATRIX_C_OFFSET + (i*BLOCK_SIZE + x)*matrix_size + j*BLOCK_SIZE + y] + temp;
                    }
                }

            }

        }

    }


}



int main()
{
	xthread_t t_id[NUMOFCORE];

	xil_printf("\nEntering the main\n\r");
	// prepare memory
	xil_printf("\nStarting \n\r");
	create_gemm();
	xil_printf("\n Matrixes are created\n\r");
	//print_gemm();
	timer_helper_start();

	for(int i=0;i<NUMOFCORE;i++){
		xthread_create_main(&t_id[i],(int*)(0x44A20000 + i*0x00010000),(int)control_unit_1_0,(int)mem_float);
		//xil_printf("\nCore %d - Create",t_id[i].core_number);
	}

	for(int i=0;i<NUMOFCORE;i++){
		xthread_join_main(&t_id[i]);
		//xil_printf("\nCore %d - Join",t_id[i].core_number);
	}

	timer_helper_stop();


	print_gemm();

	xil_printf("\n\rExiting the main\n\r");

	return 0;

}
