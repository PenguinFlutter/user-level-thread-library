#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"
#include "queue.h"

//possible states
#define ready   1
#define running 2
#define blocked 3
#define zombie  4

queue_t readyQ;
queue_t zombieQ;
struct uthread_tcb *curThread; //currently running thread

struct uthread_tcb {
	int state;
	uthread_ctx_t *context;
	void *stack;
};

//get the currently executing thread
struct uthread_tcb *uthread_current(void)
{
	return curThread;
}

//Yield PU to next thread in the ready queue
void uthread_yield(void)
{
	preempt_disable(); //disable preempt since we're messing with threads
    struct uthread_tcb *prevThread;
	prevThread = curThread;

	int retval = queue_dequeue(readyQ, (void**)&curThread);

	if (retval == -1) {
		return;
	}

	curThread->state = running; 

	if (prevThread->state == 2) { // if previous thread was also running;
		prevThread->state = ready;
	}
	
	//Enqueue previous thread back into ready queue
	retval = queue_enqueue(readyQ, prevThread);

	preempt_enable(); //enable preempt

	if (retval == -1) {
		return;
	}

	//switch context to next thread
	uthread_ctx_switch(prevThread->context, curThread->context);

}

//set current state to zombie and update zombieq
void uthread_exit(void) {
	preempt_disable(); //disable preempt since we're manipulating zombieQ
	curThread->state = zombie;
	
	//Enqueue current thread into zombie queue
	int retval = queue_enqueue(zombieQ, curThread);  

	preempt_enable(); //enable preempt

	if (retval == -1) {
		return;
	}

	//yield to next thread
	uthread_yield();
}

//create a new thread with function and argument
int uthread_create(uthread_func_t func, void *arg)
{
	preempt_disable(); //disable preempt since we're manipulating readyQ

    struct uthread_tcb *newThread = (struct uthread_tcb*)malloc(sizeof(struct uthread_tcb)); 	//allocate memory for a thread
	newThread->stack = uthread_ctx_alloc_stack();		 //allocate stack segment 
    newThread->context = (uthread_ctx_t *)malloc(sizeof(uthread_ctx_t)); 	//allocate memory for context of the thread
	newThread->state = ready; 	//set the state of the new thread to ready

    int retval = uthread_ctx_init(newThread->context, newThread->stack, func, arg); 	//initialize the uthread's context

	//if retval equals -1, then we encountered an error
	if (retval == -1) {
		return -1;
	}

	retval = queue_enqueue(readyQ, newThread); // enqueue the newthread into ready queue
	preempt_enable(); //enable preempt

	if (retval == -1) {
		return -1;
	}

    return 0;
}

//beginning of uthread process
int uthread_run(bool preempt, uthread_func_t func, void *arg)
{
	preempt_start(preempt);

	//initialize queues
    readyQ = queue_create();
    zombieQ = queue_create();

	//set up thread
	curThread = (struct uthread_tcb *)malloc(sizeof(struct uthread_tcb));
	curThread->context = (uthread_ctx_t *)malloc(sizeof(uthread_ctx_t));
	curThread->state = ready;

	int retval = queue_enqueue(readyQ, curThread); //enqueue thread into ready queue
	
	if (retval == -1) {
		return -1;  
	} else {

		retval = uthread_create(func, arg); //new uthread

		if (retval == -1) {
			return -1;
		}

		//yield to all threads until threads in ready queue has executed
		for (int size = queue_length(readyQ); size > 0; size = queue_length(readyQ)) {
			uthread_yield();
		}

	}

	//free up allocated memory
	for(int size = queue_length(zombieQ); size > 0; size--) {

		queue_dequeue(zombieQ, (void**)&curThread);

		uthread_ctx_destroy_stack(curThread->stack);
		free(curThread->context);
		free(curThread);
	}

	preempt_stop();
	return 0;
}

//block currently running thread
void uthread_block(void)
{
	if (curThread->state == 3) { //is blocked
		return;
    }

	curThread->state = blocked;
	uthread_yield();  // Switch to the next available thread
}

//unblock currently running thread
void uthread_unblock(struct uthread_tcb *uthread)
{
    if (uthread->state != 3) { //not blocked
		return;
    }

	uthread->state = ready;
    queue_enqueue(readyQ, uthread);
}
