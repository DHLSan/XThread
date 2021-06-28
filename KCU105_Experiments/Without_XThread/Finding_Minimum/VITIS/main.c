//send data over UART
#include "xil_printf.h"

//information about AXI peripherals
#include "xparameters.h"

// Timer
#include "xtmrctr.h"

#include <math.h>

#include "xthread.h"

int *control_unit_1_0 = (int *) 0x44A10000;
int *core_1_0 = (int *) 0x44A20000;
float *mem_float = (float *) 0x80000000; // write purpose

/*** Algorithm Specific ***/

#define NUMOFCORE 1

#define DATA_SIZE 20000
#define MINIMUM_VALUE_OFFSET 0
#define PARTIAL_LIST_SIZE_OFFSET 1
#define LIST_OFFSET 2

void create_int_sort(){

	mem_float[MINIMUM_VALUE_OFFSET] = DATA_SIZE + 1;

	mem_float[PARTIAL_LIST_SIZE_OFFSET] = ((float) DATA_SIZE) / ((float) NUMOFCORE);

	for(int i=0;i<DATA_SIZE;i++){
		mem_float[LIST_OFFSET + i] = i + 100;
	}
	mem_float[2000]=5;

}

void print_int_sort(){

	xil_printf("\n\rMin: %d",(int) mem_float[MINIMUM_VALUE_OFFSET]);

	int whole, millionths;
	whole = mem_float[PARTIAL_LIST_SIZE_OFFSET];
	millionths = (mem_float[PARTIAL_LIST_SIZE_OFFSET] - whole) * pow(10.0, 6); // take 6 digits
	xil_printf("\n\rPartial Size: %d,%06d \n",whole, millionths);
}

/******/

void setup(){
	// Give address of the first core to the controller, thus the controller knows where to start counting
	control_unit_1_0[12] = (int) core_1_0;
}

void print_cores(){

	xil_printf("\ncu: %d - %X - %d - %d - %X",control_unit_1_0[0],control_unit_1_0[4],control_unit_1_0[6],control_unit_1_0[8],control_unit_1_0[10]);

	for(int i=0;i<8;i++){
		int* c = (int*)(0x44A20000 + i*0x00010000);
		xil_printf("\ncore_%d: %d - %X - %d - %d - %X",i,c[0],c[4],c[6],c[8],c[10]);
	}

}

/******/
void runMicroblaze(){
	float next_element;
	float my_min = 1500;

	for (int counter = 0; counter <= DATA_SIZE; counter++){
			next_element = mem_float[LIST_OFFSET + counter];
			if (next_element < my_min)
				my_min = next_element;
	}

	  mem_float[MINIMUM_VALUE_OFFSET] = my_min;
}
int main()
{
	xthread_t t_id[NUMOFCORE];

	setup();

	xil_printf("\nEntering the main\n\r");

	// prepare memory

	create_int_sort();

	// -------------------------------------------------
	timer_helper_start();

	for(int i=0;i<NUMOFCORE;i++){
		xthread_create_main(&t_id[i],(int*)(0x44A20000 + i*0x00010000),(int)control_unit_1_0,(int)mem_float);
		//xil_printf("\nCore %d - Create",t_id[i].core_number);
	}

	for(int i=0;i<NUMOFCORE;i++){
		xthread_join_main(&t_id[i]);
		//xil_printf("\nCore %d - Join",t_id[i].core_number);
	}

	//runMicroblaze();
	timer_helper_stop();

	print_int_sort();

	//print_cores();

	xil_printf("\n\rExiting the main\n\r");

	return 0;

}
