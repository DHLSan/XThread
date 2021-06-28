//send data over UART
#include "xil_printf.h"

//information about AXI peripherals
#include "xparameters.h"
#include "timerHelper.h"
// Timer
#include "xtmrctr.h"

#include <math.h>

#include "xthread.h"

int *control_unit_1_0 = (int *) 0x44A10000;
int *core_1_0 = (int *) 0x44A20000;
float *mem_float = (float *) 0x80000000; // write purpose

/*** Algorithm Specific ***/

#define NUMOFCORE 1
#define NUMOFVAR 31

#define NUMOFCORE_OFFSET 0
#define NUMOFVAR_OFFSET 1
#define PIVOT_OFFSET 2
#define MATRIX_OFFSET 16

void create_prime_matrix(){
	float prime = 2;

	    for(int i=0;i<NUMOFVAR;i++){
	        for(int j=0;j<NUMOFVAR+1;j++){

	        	mem_float[17000 + (NUMOFVAR+1)*i + j] = prime;

	            for(int k=prime+1;;k++){

	                for(int g=2;g<k;g++){
	                    if( k % g == 0) break;
	                    if( g == k-1 ) prime = k;
	                }
	                if(prime==k) break;

	            }

	        }
	    }

		xil_printf("\n\rcreated prime matrix\n\r");
}

void create_gaussian(){

	mem_float[NUMOFCORE_OFFSET] = NUMOFCORE;
	mem_float[NUMOFVAR_OFFSET] = NUMOFVAR;


	    for(int i=0;i<NUMOFVAR*(NUMOFVAR+1);i++){
	    	mem_float[MATRIX_OFFSET + i] = mem_float[17000 + i];
	    }

		xil_printf("\n\rcreated gaussian\n\r");
}
void run_gaussian(int core_number, float *mem){

	float numofcore = mem[NUMOFCORE_OFFSET];
	float numofvar_f = mem[NUMOFVAR_OFFSET];
	int numofvar_i =  numofvar_f;

	int pivot = 0; // start point of pivot is 0

	while(pivot < numofvar_i){

		 for(int current=(numofvar_f/numofcore)*core_number; current<=(numofvar_f/numofcore)*(core_number+1)-1; current++){

			if(current==pivot) continue; // jump over the row if it is pivot

			float multfact = mem[MATRIX_OFFSET + (numofvar_i+1)*current + pivot] / mem[MATRIX_OFFSET + (numofvar_i+1)*pivot + pivot];
			for(int i=pivot;i<=numofvar_i;i++){
				float temp1 = mem[MATRIX_OFFSET + (numofvar_i+1)*pivot + i];
				mem[MATRIX_OFFSET + (numofvar_i+1)*current + i] = mem[MATRIX_OFFSET + (numofvar_i+1)*current + i] - multfact * temp1;
			}

		}
		mem[PIVOT_OFFSET] = mem[PIVOT_OFFSET]  + 1; // increase pivot

		pivot = mem[PIVOT_OFFSET]; // read pivot

	}
}
void print_gaussian(){

	xil_printf("\n");
	xil_printf("\rnumofcore: %d \n",(int) mem_float[NUMOFCORE_OFFSET]);
	xil_printf("\rnumofvar: %d \n",(int) mem_float[NUMOFVAR_OFFSET]);
	xil_printf("\n\r");

	for(int i=0;i<NUMOFVAR;i++){
		for(int j=0;j<NUMOFVAR+1;j++){

			float f = mem_float[MATRIX_OFFSET + (NUMOFVAR+1)*i + j];
			if(f < 1 * pow(10.0, -3)  && f > -1 * pow(10.0, -3)){
				xil_printf(" 0 |"); // round to "0"
				continue;
			}

			int whole, millionths;
			char sign = ' ';
			whole = f;
			millionths = (f - whole) * pow(10.0, 6); // take 6 digits
			if(f<0){
				sign = '-';
				millionths = (-1) * millionths;
			}
			xil_printf("%c%d.%06d |",sign, abs(whole), millionths);

		}
		xil_printf("\n\r");
	}

	xil_printf("\n\r");

}

/******/

void print_cores(){

	xil_printf("\ncu: %d - %X - %d - %d - %d - %X",control_unit_1_0[0],control_unit_1_0[4],control_unit_1_0[6],control_unit_1_0[8],control_unit_1_0[10],control_unit_1_0[12]);

	for(int i=0;i<NUMOFCORE;i++){
		int* c = (int*)(0x44A20000 + i*0x00010000);
		xil_printf("\n\rcore_%d: %d - %X - %d - %d - %X- %X- %X",i,c[0],c[4],c[6],c[8],c[10],c[11]);
	}

}

void setup(){
	// Give address of the first core to the controller, thus the controller knows where to start counting
	control_unit_1_0[12] = (int) core_1_0;
}

int main()
{
	xthread_barrier_init_main(control_unit_1_0,NUMOFCORE); // init barrier_0 - holds all cores
	xthread_t t_id[NUMOFCORE];
	setup();

	xil_printf("\nEntering the main\n\r");
	// prepare memory
	xil_printf("\nStarting \n\r");
	create_prime_matrix(); // one time is enough
	create_gaussian();
	xil_printf("\nFinished\n\r");
	print_cores();
	timer_helper_start();

	for(int i=0;i<NUMOFCORE;i++){
		xthread_create_main(&t_id[i],(int*)(0x44A20000 + i*0x00010000),(int)control_unit_1_0,(int)mem_float);
		//xil_printf("\nCore %d - Create",t_id[i].core_number);
	}

	for(int i=0;i<NUMOFCORE;i++){
		xthread_join_main(&t_id[i]);
		//xil_printf("\nCore %d - Join",t_id[i].core_number);
	}

    //run_gaussian(0,mem_float);
	timer_helper_stop();
	print_cores();

	print_gaussian();
	xil_printf("\n\rExiting the main\n\r");

	return 0;

}
