#include <stdio.h>

#define RELIABILITY 0

#define COREDISTANCE_INT 0x00010000 / 4
#define ACK_OFFSET_INT 9

#define MAXNUMOFCORE 16
#define MAXMECHTYPE 3 // 3 for 3 different queue - mutex, semaphore, barrier -
#define GENERALMAX 16 // should be equal to MAXMUTEX, MAXBARRIER, MAXSEMAPHORE

/*** MUTEX ***/

#define MAXMUTEX 16

int queue_mutex[MAXMUTEX][MAXNUMOFCORE-1];
int acquired_mutex[MAXMUTEX];

/************/

/*** BARRIER ***/

#define MAXBARRIER 16

int count_barrier[MAXBARRIER];

int await_barrier[MAXBARRIER]; // keep tracking number of awaiting cores
int queue_barrier[MAXBARRIER][MAXNUMOFCORE-1];

/***************/

/*** SEMAPHORE ***/

#define MAXSEMAPHORE 16

int count_semaphore[MAXSEMAPHORE];

int inside_semaphore[MAXSEMAPHORE]; // keep tracking how many cores inside
int queue_semaphore[MAXSEMAPHORE][MAXNUMOFCORE-1];

/***************/

int queue_last_index[MAXMECHTYPE][GENERALMAX];

/***************/

// pop out the first element
int pop(int mech_type, int mech_address, int queue[][MAXNUMOFCORE-1]){

	/*
	 * mech_type :
	 * 0 - mutex
	 * 1 - barrier
	 * 2 - semaphore
	 */

	int head = (MAXNUMOFCORE == 1) ? 0 : queue[mech_address][0];

	for(int i=0; i<MAXNUMOFCORE-1-1; i++){
#pragma HLS pipeline II=1
#pragma HLS unroll factor=16
		queue[mech_address][i] = queue[mech_address][i+1];
	}

	// if head is 0, there is no core in queue
	/** reset the tail */
	if(head != 0){
		queue[mech_address][MAXNUMOFCORE-1-1] = 0;
		// decrease last available index
		queue_last_index[mech_type][mech_address]--;
	}

	return head;

}

void push(int mech_type,int mech_address, int core_address, int queue[][MAXNUMOFCORE-1]){

	/*
	 * mech_type :
	 * 0 - mutex
	 * 1 - barrier
	 * 2 - semaphore
	 */

	queue[mech_address][queue_last_index[mech_type][mech_address]] = core_address;

	// increase last available index
	queue_last_index[mech_type][mech_address]++;


	/*for(int i=0;i<MAXNUMOFCORE-1;i++){
#pragma HLS pipeline II=1
//#pragma HLS unroll factor=16
		if(queue[mech_address][i] == 0) {
			queue[mech_address][i] = core_address;
			return;
		}
	}*/

}

void inform(int core_number, volatile int *out_core){

#if RELIABILITY == 1
	while(*(out_core + (COREDISTANCE_INT * core_number) + ACK_OFFSET_INT) != 0)
#endif
		*(out_core + (COREDISTANCE_INT * core_number) + ACK_OFFSET_INT) = 0;
}

void control_unit_1(/* Arguments */ int core_number, volatile int *instruc_type, volatile int *mech_address,
		            /* Flag */ volatile int *go,
		            /* Core */ volatile int *out_core){

#pragma HLS RESOURCE variable=acquired_mutex core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=queue_mutex core=RAM_1P_BRAM

#pragma HLS RESOURCE variable=count_barrier core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=queue_barrier core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=await_barrier core=RAM_1P_BRAM

#pragma HLS INTERFACE s_axilite port=return bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=core_number bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=mech_address bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=instruc_type bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=go bundle=BUS_A

#pragma HLS INTERFACE m_axi port=out_core offset=slave bundle=BUS_B
#pragma HLS INTERFACE s_axilite port=out_core bundle=BUS_A


	// Initialize instruction - answer no one
	if(core_number < 0){

		// Initialize Barrier
		if(*instruc_type == 2){
			count_barrier[*mech_address] = (-1) * core_number; // for now, assume core number as count variable
			return;
		}

		// Initialize Semaphore

		if(*instruc_type == 3){
			count_semaphore[*mech_address] = (-1) * core_number;
			return;
		}

		return; // return anyway for avoiding any mistakes
	}

	///////////////////////////////////////////////////////////////

	// reliability - inform core that it has been accepted
#if RELIABILITY == 1
	while( *(out_core + (COREDISTANCE_INT * core_number) + ACK_OFFSET_INT) != 1)
#endif
		*(out_core + (COREDISTANCE_INT * core_number) + ACK_OFFSET_INT) = 1;

	// wait for the signal to interpret
	while(*go != 1);

	// tell the core that go signal can be reseted
#if RELIABILITY == 1
	while( *(out_core + (COREDISTANCE_INT * core_number) + ACK_OFFSET_INT) != 2)
#endif
		if(*go == 1)
					*(out_core + (COREDISTANCE_INT * core_number) + ACK_OFFSET_INT) = 2;


	// wait for resetting go
	while(*go != 0 || *instruc_type == 20); // *instruc_type is never 20


	/*
	 * Interpretation Section - consider given instruction and generate an appropriate behaviour
	 */

	// MUTEX LOCK - instruc_type: 0
	if(*instruc_type == 0){

		// Case: lock is available -> ACQUIRE
		if(acquired_mutex[*mech_address] == 0){
			acquired_mutex[*mech_address] = core_number + 1; // save it by increasing 1

			// inform
			inform(core_number,out_core);

			return;
		}

		// Case: lock is already locked by 'ANOTHER' core -> QUEUE
		if(acquired_mutex[*mech_address] != core_number + 1){
			push(0,*mech_address,core_number+1,queue_mutex); // save it by increasing 1

			return;
		}

		// Note: cover the case core_number == acquired_mutex[*mech_address] | lock after lock

	}


	// MUTEX UNLOCK - instruc_type: 1
	if(*instruc_type == 1){

		// update acquired_mutex
		int head = pop(0,*mech_address,queue_mutex);
		acquired_mutex[*mech_address] = head;

		// inform the core which sent the unlock request
		inform(core_number,out_core);

		// if there is another core in queue, inform it - P.S: remember head is actually core_number + 1
		if(head != 0) inform(head-1,out_core);

		return;
	}


	// BARRIER - instruc_type: 2
	if(*instruc_type == 2){

		// if barrier is FULL
		if(await_barrier[*mech_address] == count_barrier[*mech_address] - 1){

			inform(core_number,out_core); // inform current one

			int head;
			while(1){
				head = pop(1,*mech_address,queue_barrier);
				if(head == 0) break;
				inform(head-1,out_core); // inform others
			}

			await_barrier[*mech_address] = 0; // reset

			return;

		}

		// if barrier is not full
		if(await_barrier[*mech_address] < count_barrier[*mech_address] - 1){

			push(1,*mech_address,core_number+1,queue_barrier); // save it by increasing 1
			await_barrier[*mech_address]++; // increase 1

			return;

		}

	}


	// SEMAPHORE WAIT - instruc_type: 3
	if(*instruc_type == 3){

		// if semaphore is not full
		if(inside_semaphore[*mech_address] != count_semaphore[*mech_address]){

			inside_semaphore[*mech_address]++;
			inform(core_number,out_core); // give the core semaphore

			return;
		}

		// if semaphore is full
		if(inside_semaphore[*mech_address] == count_semaphore[*mech_address]){

			push(2,*mech_address,core_number+1,queue_semaphore);

			return;
		}

	}


	// SEMAPHORE POST - instruc_type: 4
	if(*instruc_type == 4){

		inside_semaphore[*mech_address]--;

		inform(core_number,out_core); // inform current one

		int head = pop(2,*mech_address,queue_semaphore);
		if(head != 0){
			inform(head-1,out_core); // inform next one
			inside_semaphore[*mech_address]++;
		}

	}

}
