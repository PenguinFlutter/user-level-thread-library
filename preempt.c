#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ucontext.h>

#include "private.h"
#include "uthread.h"

/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
/* References
https://www.gnu.org/software/libc/manual/2.36/html_mono/libc.html#Signal-Actions
https://www.gnu.org/software/libc/manual/2.36/html_mono/libc.html#Handler-Returns
https://www.gnu.org/software/libc/manual/2.36/html_mono/libc.html#Setting-an-Alarm
*/

struct itimerval timer; //global var for timer interval
struct sigaction sa; //global var for signal handling
#define HZ 100

//Signal handler for preemption
void preempt_handler() {
    
    uthread_yield(); //yield to next when when signal is received
}

//block signal
void preempt_disable(void)
{
    sigset_t alarm;
    sigemptyset(&alarm); //initialize signal mask
    sigaddset(&alarm, SIGVTALRM); //add SIGVTALRM to the set

	//block SIGVTALRM signal
    sigprocmask(SIG_BLOCK, &alarm, NULL);
}

//unblock signal
void preempt_enable(void)
{
    sigset_t alarm;
    sigemptyset(&alarm); //initialize signal mask
    sigaddset(&alarm, SIGVTALRM); //add SIGVTALRM to the set

	//unblock SIGVTALRM signal
    sigprocmask(SIG_UNBLOCK, &alarm, NULL);
}

//start preemption
void preempt_start(bool preempt)
{
    if (preempt == true) {

        //intialize signal action
        sa.sa_handler = &preempt_handler;
        sigaction(SIGVTALRM, &sa, NULL);

        // Set timer interval
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = 1000000 / HZ;
        timer.it_interval = timer.it_value;

        // Start the timer
        setitimer(ITIMER_VIRTUAL, &timer, NULL);
    }

}

//end preemption
void preempt_stop(void)
{
    // reset timer and signal
    timer.it_value.tv_sec = 0; 
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);

	sigaction(SIGVTALRM, &sa, NULL);

}
