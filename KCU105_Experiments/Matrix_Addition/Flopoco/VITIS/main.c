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

void run_ma(int core_number, float *mem){

    float numofcore = mem[NUMOFCORE_OFFSET];
	int matrix_size =  mem[MATRIX_SIZE_OFFSET];

    int start = (matrix_size/numofcore)*core_number;
    int end = (matrix_size/numofcore)*(core_number+1);

    int matrix_A_offset = MATRIX_START_OFFSET;
    int matrix_B_offset = matrix_A_offset + matrix_size * matrix_size;
    int matrix_C_offset = matrix_B_offset + matrix_size * matrix_size;

    int size_i = matrix_size;

    for (int i = start; i < end; i++) {
        for (int j = 0; j < matrix_size; j++) {
            mem[matrix_C_offset + (size_i * i) + j]=mem[matrix_A_offset + (size_i * i) + j]+mem[matrix_B_offset + (size_i * i) + j];
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

	//run_ma(0,mem_float);
	timer_helper_stop();


	print_gemm();

	xil_printf("\n\rExiting the main\n\r");

	return 0;

}
