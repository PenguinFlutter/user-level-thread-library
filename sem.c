#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "private.h"

#define ERROR -1
#define SUCCESS 0

struct semaphore {
	size_t count;
	queue_t blocked;
};

sem_t sem_create(size_t count)
{
	// allocate our semaphore
	sem_t new_semaphore = (sem_t)malloc(sizeof(struct semaphore));

	// initialize count as specified
	new_semaphore -> count = count;
	
	// and create a queue
	new_semaphore -> blocked = queue_create();

	// return this semaphore
	return new_semaphore;
}

int sem_destroy(sem_t sem)
{
	// check if sem is NULL
	if (!sem) {
		return ERROR;
	}

	// destroy our queue first
	if (queue_destroy(sem -> blocked)) {
		// if we couldn't destroy it (there are threads being blocked), err
		return ERROR;
	}

	// then free our semaphore
	free(sem);

	return SUCCESS;
}

int sem_down(sem_t sem)
{
	// check if sem is NULL
	if (!sem) {
		return ERROR;
	}

	while (1) {
		// is there an available resource
		if (sem -> count > 0) {
			// if so, take it!
			sem -> count--;

			return SUCCESS;
		}
		

		// if not we should block the current thread and wait till it is available
		queue_enqueue(sem -> blocked, uthread_current());
		uthread_block();
	}

}

int sem_up(sem_t sem)
{
	// check if sem is NULL
	if (!sem) {
		return ERROR;
	}

	// free the resource
	sem -> count++;

	// are any threads waiting for a resource? 
	if (queue_length(sem -> blocked) > 0) {
		// wake up the first in line
		struct uthread_tcb *toWake;
		queue_dequeue(sem -> blocked, (void**)&toWake);
		uthread_unblock(toWake);
		uthread_yield();
	}

	return SUCCESS;
}

