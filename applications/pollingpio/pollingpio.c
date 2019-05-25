#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <limits.h>
#include <wiringPi.h>

#define REALTIME_TASK

#define sleep_ms 4
#define STOP 100000

long tout = sleep_ms*1000*1000;


void nsleep(struct timespec deadline)
{
    // Add the time you want to sleep
  	deadline.tv_nsec += tout;
  	// Normalize the time to account for the second boundary
  	if(deadline.tv_nsec >= 1000000000) {
  	    deadline.tv_nsec -= 1000000000;
  	    deadline.tv_sec++;
  	}
  	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
}

void pollingpio_task(int pin, int val)
{
    struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);
		digitalWrite(pin, val); 
    nsleep(start);
}

void set_run_cpu(int cpu)
{
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
    CPU_SET(cpu, &my_set);     /* set the bit that represents core 7. */
    if (sched_setaffinity(0, sizeof(my_set), &my_set) == -1)
    {
        printf("err sched_setaffinity"); exit(1);
    }
}

void *pollingpio_thread(void* data)
{
		wiringPiSetup();
		int pin = 0;
		pinMode(pin, OUTPUT);
    int value = 0;
		int k = 0;
    while (k<STOP)
    {
				pollingpio_task(pin, value);
				value = (value == 0) ? 1: 0;
				k++;
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;
    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }
    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        goto out;
    }
    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret) {
        printf("pthread setstacksize failed\n");
        goto out;
    }
#ifdef REALTIME_TASK
    /* Set scheduler policy and priority of pthread */
    printf("Setting thread to real-time task\n");
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        printf("pthread setschedpolicy failed\n");
        goto out;
    }
    param.sched_priority = 98;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        printf("pthread setschedparam failed\n");
        goto out;
    }
    /* Use scheduling parameters of attr */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        printf("pthread setinheritsched failed\n");
        goto out;
    }
#endif
    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread, &attr, pollingpio_thread, NULL);
    if (ret) {
        printf("create pthread failed: %d\n", ret);
        goto out;
    }
    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret)
        printf("join pthread failed: %m\n");

out:
    return ret;

}
