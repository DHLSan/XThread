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

void xthread_barrier_init_main(volatile int *control_unit, int count);

void xthread_barrier_init_core(xthread_barrier_t *b);

void xthread_semaphore_init_main(volatile int *control_unit, int count);

void xthread_semaphore_init_core(xthread_semaphore_t *s);

/**** MECHANISMS ****/

void xthread_mutex_lock(volatile int *control_unit, int core_number, volatile int *ack, xthread_mutex_t* m, float *mem);

void xthread_mutex_unlock(volatile int *control_unit, int core_number, volatile int *ack, xthread_mutex_t* m, float *mem);

void xthread_barrier_wait(volatile int *control_unit, int core_number, volatile int *ack, xthread_barrier_t* b, float *mem);

void xthread_semaphore_wait(volatile int *control_unit, int core_number,volatile int *ack, xthread_semaphore_t* s, float *mem);

void xthread_semaphore_post(volatile int *control_unit, int core_number, volatile int *ack, xthread_semaphore_t* s, float *mem);

#endif
