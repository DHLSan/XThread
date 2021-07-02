/******************************************************************************
*
* Copyright (C) 2008 - 2014 Xilinx, Inc.  All rights reserved.
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

#ifndef xthread

#define xthread

typedef struct xthread {
	int core_number;
	int core_address;
	int control_unit_address;
	int memory_address;
} xthread_t;

typedef struct xthread_mutex {
	int is_initiliazed;
	int mutex_no;
} xthread_mutex_t;

typedef struct xthread_barrier {
	int is_initiliazed;
	int barrier_no;
	int count;
} xthread_barrier_t;

typedef struct xthread_semaphore {
	int is_initiliazed;
	int semaphore_no;
	int count;
} xthread_semaphore_t;

/*
 * returns "0" if creating is successfully.
 * returns "-1" if MAXTHREAD value is reached.
 * returns "1" if there is a core running on given address.
 */
int xthread_create_main(xthread_t* th, int* core_address, int control_unit_address, int memory_address);

int xthread_join_main(xthread_t* th);

/**** INITIALIZERS ****/

/*
 * It is a crucial point that barriers and semaphores must
 * be initialized in both the main thread and
 * cores. Also they must be initialized in the
 * same order in both the main thread and cores.
 */

void xthread_mutex_init_core(xthread_mutex_t *m);

void xthread_barrier_init_main(int *control_unit, int count);

void xthread_barrier_init_core(xthread_barrier_t *b);

void xthread_semaphore_init_main(int *control_unit, int count);

void xthread_semaphore_init_core(xthread_semaphore_t *s);

/**** MECHANISMS ****/

void xthread_mutex_lock(int *control_unit, int core_number, int *ack, xthread_mutex_t* m, float *mem);

void xthread_mutex_unlock(int *control_unit, int core_number, int *ack, xthread_mutex_t* m, float *mem);

void xthread_barrier_wait(int *control_unit, int core_number, int *ack, xthread_barrier_t* b, float *mem);

void xthread_semaphore_wait(int *control_unit, int core_number, int *ack, xthread_semaphore_t* s, float *mem);

void xthread_semaphore_post(int *control_unit, int core_number, int *ack, xthread_semaphore_t* s, float *mem);

#endif
