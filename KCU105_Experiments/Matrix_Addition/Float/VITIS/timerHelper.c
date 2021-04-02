#include "xtmrctr.h"
#include <limits.h>
#include <math.h>

//send data over UART
#include "xil_printf.h"
#include "timerHelper.h"
#define TIMER_CNTR_0 0
#define TIMER_CNTR_1 1
XTmrCtr TimerCounter;

unsigned long int timer_counter0;
unsigned long int timer_counter1;


void timer_helper_start(){
	// init
	XTmrCtr_Initialize(&TimerCounter, XPAR_TMRCTR_0_DEVICE_ID);
	// enable cascade mode
	XTmrCtr_SetOptions(&TimerCounter,TIMER_CNTR_0, XTC_CASCADE_MODE_OPTION);
	// reset counters
	XTmrCtr_Reset(&TimerCounter, TIMER_CNTR_0);
	XTmrCtr_Reset(&TimerCounter, TIMER_CNTR_1);
	// print
	xil_printf("\ntimer is on\n\r");
	// start
	XTmrCtr_Start(&TimerCounter, TIMER_CNTR_0);
}

void print_time(unsigned long int counter0,unsigned long int counter1){
	double counter0_time = (double)counter0 / XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ;
	double counter1_time = (counter1* (UINT_MAX / XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ));
	double elapsed_time = counter1_time + counter0_time;
	int whole, millionths;
	whole = elapsed_time;
	millionths = (elapsed_time - whole) * pow(10.0, 6); // take 6 digits
	xil_printf("\nTime: %d,%06d \n",whole, millionths);
}

void timer_helper_stop(){
	// stop
	XTmrCtr_Stop(&TimerCounter, TIMER_CNTR_0);
	// print
	xil_printf("\ntimer is off\n\r");
	// retrieve counter value
	timer_counter0 = XTmrCtr_GetValue(&TimerCounter, TIMER_CNTR_0);
    timer_counter1 = XTmrCtr_GetValue(&TimerCounter, TIMER_CNTR_1);
	// print time
    print_time(timer_counter0,timer_counter1);
}
