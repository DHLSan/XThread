/******************************************************************************
*
* Copyright (C) 2010 - 2015 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

#include "xparameters.h"
#include "xil_cache.h"

#include "platform_config.h"

#include "xthread.h"

#define RELIABILITY 1

/*
 * Following offsets are strictly dependent on
 * how control unit is written. They may need
 * to be updated after inputs and outputs of
 * control unit are changed or any change in
 * how they are placed in order is made.
 *
 * Related information can be found in
 * "xcontrol_unit_hw.h" file under drivers
 * folder of the certain implementation
 * of control unit.
 *
 */

#define CORE_NUMBER_OFFSET_INT 4
#define INSTRUC_TYPE_OFFSET_INT 6
#define MECH_ADDRES_OFFSET_INT 8
#define GO_OFFSET_INT 10

/**** BASICS ****/

#define MAXNUMOFCORE 16

#define MAXMUTEX 16
#define MAXBARRIER 16
#define MAXSEMAPHORE 16

int created_thread_counter = 0;

int created_mutex_counter = 0;
int created_barrier_counter = 0;
int created_semaphore_counter = 0;

void fill_xthread_t(xthread_t* th, int* core_address){
	th -> core_number = core_address[7];
	th -> core_address = (int) core_address;
	th -> control_unit_address = core_address[4];
	th -> memory_address = core_address[11];
}

int xthread_create_main(xthread_t* th, int* core_address, int control_unit_address, int memory_address){

	if(created_thread_counter == MAXNUMOFCORE) return -1; // if MAXTHREAD value is reached.

	if(core_address[0] != 4 && core_address[0] != 6) {

		fill_xthread_t(th,core_address);


		created_thread_counter++;

		return 1; // if there is a core running on given address.
	}

	core_address[4] = control_unit_address; // address of of the controller unit
	core_address[7] = created_thread_counter; // core number of the core
	core_address[11] = memory_address; // address of ddr2 sdram

	fill_xthread_t(th,core_address);

	core_address[0] = 1; // start given core

	created_thread_counter++;

	return 0;
}

int xthread_join_main(xthread_t* t_id){

	// here this is critical since without volatile it just hangs due to a compiler optimization being applied by Vitis
	// Vitis does not know this address is being written by a peripheral, so we have to tell the compiler this address may change by some hardware core
	volatile int* core_address;
	core_address = (int*) t_id -> core_address;

	while(core_address[0] != 4);

	return 0;
}

/**** INITIALIZERS ****/

void INIT(int *control_unit, int core_number, int instruc_type, int mech_address){

	/*
	 * In this function, it is assumed that no
	 * other thread is running. Thus, there is
	 * no flag capturing and this function can
	 * do its work safely.
	 */

#if RELIABILITY == 1
	while(*(control_unit + CORE_NUMBER_OFFSET_INT ) != core_number)
#endif
		*(control_unit + CORE_NUMBER_OFFSET_INT ) = core_number;

#if RELIABILITY == 1
	while(*(control_unit + INSTRUC_TYPE_OFFSET_INT) != instruc_type)
#endif
		*(control_unit + INSTRUC_TYPE_OFFSET_INT) = instruc_type;

#if RELIABILITY == 1
	while(*(control_unit + MECH_ADDRES_OFFSET_INT) != mech_address)
#endif
		*(control_unit + MECH_ADDRES_OFFSET_INT) = mech_address;

	*(control_unit) = 1; // give start signal

	/*
	 * Control unit knows what to do, no need
	 * to wait an acknowledgment.
	 */

}

void xthread_barrier_init_main(int *control_unit, int count){

	if(created_barrier_counter == MAXBARRIER) return;

	// Instruction Code for BARRIER INIT : 2
	INIT(control_unit,(-1)*count,2,created_barrier_counter);

	created_barrier_counter++;

}

void xthread_barrier_init_core(xthread_barrier_t *b){

	if(b -> is_initiliazed == 1) return;

	if(created_barrier_counter == MAXBARRIER) return;

	b -> barrier_no = created_barrier_counter;
	b -> is_initiliazed = 1;

	created_barrier_counter++;
}

void xthread_mutex_init_core(xthread_mutex_t *m){

	if(m -> is_initiliazed == 1) return; // delete this line and is_initiliazed in struct xthread_mutex

	if(created_mutex_counter == MAXMUTEX) return;

	m -> mutex_no = created_mutex_counter;
	m -> is_initiliazed = 1;

	created_mutex_counter++;

}

void xthread_semaphore_init_main(int *control_unit, int count){

	if(created_semaphore_counter == MAXSEMAPHORE) return;

	// Instruction Code for SEMAPHORE INIT : 3
	INIT(control_unit,(-1)*count,3,created_semaphore_counter);

	created_semaphore_counter++;

}

void xthread_semaphore_init_core(xthread_semaphore_t *s){

	if(s -> is_initiliazed == 1) return;

	if(created_semaphore_counter == MAXSEMAPHORE) return;

	s -> semaphore_no = created_semaphore_counter;
	s -> is_initiliazed = 1;

	created_semaphore_counter++;

}

/**** MECHANISMS ****/

/*
 * Arguments of function "TALK" must satisfy
 * following conditions:
 *
 * 1) "return" of the function must use
 *    "s_axilite" as interface. This "return"
 *    channel helps main thread in "microblaze"
 *    to start the cores and join them.
 *
 * 2) "core_number" must use "s_axilite" as
 *    interface. Core number of each core is
 *    provided by main thread in "microblaze"
 *
 * 3) "ack" must use "s_axilite" as interface.
 *    This is a flag which makes the core
 *    informed by the control unit.
 *
 * 4) Offset value of the control unit must be
 *    given by main thread in "microblaze".
 *    To let this action work, "control_unit"
 *    must use "s_axilite" with offset slave
 *    as interface. This pointer helps the core
 *    to communicate with the control unit.
 *
 */

void TALK(int *control_unit, int core_number, int instruc_type, int mech_address, volatile int *ack, float *mem){

#pragma HLS inline off

	/*
	 * ack : 0 -> initial position and acquiring
	 * ack : 1 -> acceptance by the control unit
	 * ack : 2 -> go signal can be reseted
	 *
	 * go : 0 -> initial position
	 * go : 1 -> continue to interpret
	 *
	 */

	// Wait for acceptance
	while(*ack!=1){
		if(*(control_unit) != 1){
			*(control_unit + CORE_NUMBER_OFFSET_INT ) = core_number;
			*(control_unit) = 1; // give start signal
		}
	}

	// Reliability - Make sure that message has gone

#if RELIABILITY == 1
	while(*(control_unit + INSTRUC_TYPE_OFFSET_INT) != instruc_type)
#endif
		*(control_unit + INSTRUC_TYPE_OFFSET_INT) = instruc_type;

#if RELIABILITY == 1
	while(*(control_unit + MECH_ADDRES_OFFSET_INT) != mech_address)
#endif
		*(control_unit + MECH_ADDRES_OFFSET_INT) = mech_address;

#if RELIABILITY == 1
	// Everything is fine, set go flag = 1 for the control unit to keep going
	while(*(control_unit + GO_OFFSET_INT) != 1)
#endif
		*(control_unit + GO_OFFSET_INT) = 1;

	// Wait for ack = 2 then reset go signal
	while(*ack!=2);

#if RELIABILITY == 1
	// Reset go signal = 0
	while(*(control_unit + GO_OFFSET_INT) != 0)
#endif
		*(control_unit + GO_OFFSET_INT) = 0;

	// Wait for acquiring
	while(*ack!=0){
		if(*ack==20){ // ack is never 20
			float temp = mem[0];
			mem[0] = temp + 1;
		}
	}

}

void xthread_mutex_lock(int *control_unit, int core_number, int *ack, xthread_mutex_t* m, float *mem){

#pragma HLS inline

	// Instruction Code for LOCK : 0
	TALK(control_unit,core_number,0,m->mutex_no,ack,mem);

}

void xthread_mutex_unlock(int *control_unit, int core_number, int *ack, xthread_mutex_t* m, float *mem){

#pragma HLS inline

	// Instruction Code for UNLOCK : 1
	TALK(control_unit,core_number,1,m->mutex_no,ack,mem);

}

void xthread_barrier_wait(int *control_unit, int core_number, int *ack, xthread_barrier_t* b, float *mem){

#pragma HLS inline

	// Instruction Code for BARRIER : 2
	TALK(control_unit,core_number,2,b->barrier_no,ack,mem);

}

void xthread_semaphore_wait(int *control_unit, int core_number, int *ack, xthread_semaphore_t* s, float *mem){

#pragma HLS inline

	// Instruction Code for SEMAPHORE WAIT : 3
    TALK(control_unit,core_number,3,s->semaphore_no,ack,mem);

}

void xthread_semaphore_post(int *control_unit, int core_number, int *ack, xthread_semaphore_t* s, float *mem){

#pragma HLS inline

	// Instruction Code for SEMAPHORE POST : 4
    TALK(control_unit,core_number,4,s->semaphore_no,ack,mem);

}
