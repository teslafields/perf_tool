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
#include <sys/types.h>
#include <limits.h>

#define REALTIME_TASK

#define iter 2*5400
#define sleep_ms 100

int ITERATIONS;
long tout = sleep_ms*1000*1000;

//unsigned char result[iter];

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

//unsigned char *makepi_task()
void makepi_task()
{
    unsigned char tmp[10];
    int *r;
    int i, k;
    int b, d;
    int c = 0;
    int acc;

    r = (int *) malloc(iter*sizeof(int));

    for (i = 0; i < iter; i++) {
        r[i] = 2000;
    }

    for (k = iter; k > 0; k -= 14) {
        d = 0;

        i = k;
        for (;;) {
            d += r[i] * 10000;
            b = 2 * i - 1;

            r[i] = d % b;
            d /= b;
            i--;
            if (i == 0) break;
            d *= i;
        }
        acc = c + d / 10000;
        //sprintf(tmp, "%.4d", acc);
        //strcat(result, tmp);
        c = d % 10000;
    }
    free(r);
//    return result;
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

void *makepi_thread(void* data)
{
    printf("thread\n");

    set_run_cpu(2);

    struct timespec start;
    int ite = 0;
    while (ite < ITERATIONS)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        makepi_task();
        nsleep(start);
        ite++;
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    if ( argc < 2 )
    {
        printf("Provide iteractions\n");
        goto out;
    }
    char *endptr2;
    ITERATIONS = strtol(argv[1], &endptr2, 10);
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;
    /* Lock memory */
    /*
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }
    */
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
    ret = pthread_create(&thread, &attr, makepi_thread, NULL);
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
