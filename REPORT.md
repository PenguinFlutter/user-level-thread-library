# User-level Thread Library Report

## Project Goal
To implement a user-level threading API, with built in semaphores and preemption.

## Sources
> Note: The rubric states that we cannot have lines that are longer than 80 in
> width, but it not possible to make the links work correctly if we split them
> up. If there is a way... I couldn't find it.

- Professor's slides & lectures.
  - For all aspects of the project.
- [Stackoverflow](https://stackoverflow.com/)
  - [Suppressing unused parameter warnings](https://stackoverflow.com/questions/3599160/how-can-i-suppress-unused-parameter-warnings-in-c)
- [GNU C Library](https://www.gnu.org/software/libc/manual/)
  - [Signal actions](https://www.gnu.org/software/libc/manual/2.36/html_mono/libc.html#Signal-Actions)
  - [Handler returns](https://www.gnu.org/software/libc/manual/2.36/html_mono/libc.html#Handler-Returns)
  - [Setting alarms](https://www.gnu.org/software/libc/manual/2.36/html_mono/libc.html#Setting-an-Alarm)

## Implementation
> Note: While we will define what each implemented function of our API does, we
> will only be explaining particularly important or relevant functionality. For
> example, we do not feel that it is necessary to define how we allocated memory
> in `queue_create()`.

### Queue API
To implement our queue, we made use of a **doubly-linked list**. This gives us
the ability to perform dequeue and enqueue operations in O(1) time, in
comparison to a standard linked list.

This data structure was used for various parts of our implementation:
- **Ready queue**: Used to determine which thread should run after currently
  running thread yields.
- **Zombie queue**: Where TCB are stored for later deallocation after
  a thread exits.
- **Semaphore blocked queue**: Used for rescheduling blocked threads once we
  have resources again. 

#### Methods
- `queue_create()`: Creates a new queue on heap and returns a pointer.
- `queue_destroy()`: Deallocates a queue if it is empty.
- `queue_enqueue()`: Adds data onto the back of our queue.
  - Essentially, this function creates a new data node which points to the
    current back, but also makes the current back point to itself.
- `queue_dequeue()`: Removes data from the front of our queue, and stores it.
  - We just need to deallocate the current front node, and make it so the node
    behind it doesn't point to the old front.
  - Of course, it also needs to handle the case where it is the only node!
- `queue_delete()`: Removes specified data from our queue.
  - This one needs to iterate through our nodes to find the data first. Once it
    finds the data, we deallocate it and then need to change the nodes behind
    and in front of the deleted node to point to eachother (if they exist).
- `queue_iterate()`: Iterates through our queue, and calls some function on each
  data item.
  - Saves the address of the next node on each iteration in order to keep
    iterating in case of deletion.
- `queue_length()`: Returns the length of our queue.

#### Testing and Debugging
To test our queue API, we created a variety of tests in `queue_tester.c` in
order to make sure that the queue methods function correctly in different
conditions. Luckily, we didn't have any issues to debug!

### Uthread API
The user-level threading takes place mostly within our Uthread API. To
achieve threading, we must have a way of storing thread context and having
our threads yield control when necessary. We also will utilize the `context.c`
file in order to perform context switches.

Also, it would be good to note that our scheduling queues are stored as *global
variables*, so they can be accessed by each thread.

#### Thread Control Block (TCB)
Each thread requires a TCB, which we implemented through a struct called
`uthread_tcb`. The TCB stores the state, context, and stack for a thread. We
will store our TCB's in various [queues](#queue-api) in order to schedule our
threads.

#### Methods
- `uthread_current()`: Returns the current running thread.
  - The current thread is kept track of as a global variable.
- `uthread_yield()`: Essentially a way to voluntarily stop running a thread.
  - Changes the current thread to the next thread in the ready queue, and sets
    the state to running.
  - Changes the previously running thread to the ready state, and puts it into
    our ready queue.
  - Perform a context switch from the previous thread to the new current thread.
- `uthread_exit()`: Puts a thread into the zombie state for deallocation.
  - After changing the state, we put the TCB of the thread into our zombie
    queue.
  - Then, we `uthread_yield()` so another thread can start running.
- `uthread_create()`: Creates a new thread in the ready state, puts TCB into our
  ready queue.
- `uthread_run()`: Beginning of thread execution, initializes ready and
  zombie queues, then creates main thread and enqueues it into the ready queue. 
  - Then, it creates a new thread to execute the specified function. 
  - Repeatedly calls `uthread_yield()` until all the threads in the ready queue
  have been executed. After that, it frees up allocated memory to prevent memory
  leaks.
- `uthread_block()`: Sets the state of the thread to blocked and then calls
  `uthread_yield()` to switch to next thread. If state of thread is already
  blocked, it returns.
- `uthread_unblock()`: Sets the state of the thread to ready and enqueues it
  into the ready queue.


#### Testing and Debugging
In this program, we used the built-in testers `uthread_yield.c` and
`uthread_hello.c` to test if our program ran correctly. The two main problems we
encountered in this API were segmentation faults and infinite loops. To debug
these issues, we set up printing statements to find where the error occurred. We
then were able to identify that we were incorrectly looping `uthread_yield()` in
the `uthread_start()` function.


### Semaphore API
Semaphores actually end up being quite simple, we are just blocking our threads
when there is a lack of resources, and then resuming their execution when we
have resources to spare.

#### Semaphore structure
To implement semaphores, we first needed to define a semaphore structure. This
structure contains a *count* and a [queue](#queue-api). The count represents
available resources, and the actions of our semaphores will depend on the count.

#### Methods
- `sem_create()`: Initializes our semaphore with a specified count.
- `sem_destroy()`: Deallocates our semaphore IF we do not have any blocked
  threads in our queue.
- `sem_down()`: Causes a thread to take a resource, if one is available.
  - Checks if the resource count is above 0. If so, we decrement our count and
    do not block the thread from running.
  - If there is no available resources (count == 0), we add the current thread
    to our blocked queue, and then block it.
  - Once we are blocked, we simply wait until we are unblocked (there is a
    resource) and then take the resource.
- `sem_up()`: Frees a resource.
  - To free up a resource, we simply increment our count.
  - After freeing a resource, we should check our blocked queue. If there are
    any blocked threads, we should unblock and wake up the first one in the
    queue.
  - This causes the unblocked thread to resume in `sem_down()`, where it checks
    for resources again.

#### Testing and Debugging
To test our semaphores, we simply used the provided test scripts `sem_buffer.c`,
`sem_count.c`, `sem_prime.c`, & `sem_simple.c`. We also used gdb to help
identify issues, particularly when debugging `sem_prime.c`, where our semaphores
initially got stuck in a blocked state.

### Preemption API
Introduces an algorithm for preemptive scheduling of threads that prevents
uncooperative threads from keeping system resources for themselves. To implement
this API, we had to make use of signals & signal handlers.

#### Methods
- `preempt_handler()`: Serves as the handler for the `SIGVTALRM` signal that's
triggered by virtual timer.
- `preempt_disable()`: Disables preemption. 
  - It achieves this by blocking the `SIGVTALRM` signal using a signal mask.
- `preempt_enable()`: Enables preemption.
  - This is achieved by unblocking the `SIGVTALRM` signal with a signal mask.
- `preempt_start()`: Start of preemption. 
  - Sets up preemption by setting up the signal handler for `SIGVTALRM` and
  creating a virtual timer. 
  - If the bool "preempt" is true, the signal handler is set to
  `preempt_handler()` and the virtual time is set.
- `preempt_stop()`: Stops preemption by resetting the virtual the timer and
  restoring the signal handler.

#### Testing and Debugging
In this part, we created a simple debugging file named `test_preempt.c` that
tests if the preemptive scheduler works. Here is the functionality:
- In the main function, we call `uthread_run()` to start the threading process,
while also enabling preemption. 
- We create thread 1, which begins to run. It then creates thread 2 first.
- We run thread 1 until it yields, but it is in an infinite loop.
- However, with preemption enabled, it should be interrupted which will allow
  thread 2 to run and print a statement.
- If preemption was successful, "Penguins!" should be printed.
