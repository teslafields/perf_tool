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
#include <sys/wait.h>
#include <limits.h>

// #define REALTIME_TASK

#define iter 4*2800
#define sleep_ms 100
#define nr_tasks 10


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

void makepi2_task()
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
        c = d % 10000;
    }
    free(r);
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



void *makepi_thread(void* data)
{
    // set_run_cpu(2);
    int *id = (int *) data;

    struct timespec start;
    int ite = 0;
    while (ite < ITERATIONS)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (*id)
          makepi_task();
        else
          makepi2_task();
        nsleep(start);
        ite++;
    }
    return NULL;
}

void task(int *id)
{
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
    printf("RT Task\n");
    /* Set scheduler policy and priority of pthread */
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
    ret = pthread_create(&thread, &attr, makepi_thread, id);
    if (ret) {
        printf("create pthread failed: %d\n", ret);
        goto out;
    }
    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret)
        printf("join pthread failed: %m\n");

out:
    printf("bye ");;

}

int main(int argc, char* argv[])
{
    int tasks = 0;
    if ( argc < 2 )
    {
        printf("Provide iteractions\n");
        goto out;
    }
    else if ( argc == 3 )
    {
        char *endptr;
        tasks = strtol(argv[2], &endptr, 10);
    }

    char *endptr2;
    ITERATIONS = strtol(argv[1], &endptr2, 10);

    int identifier = 0;
    pid_t   childpid, wpid;
		int status;
    if (!tasks)
      tasks = nr_tasks;

		for (int id=0; id<tasks; id++)
		{
    	if ((childpid = fork()) == 0)
			{
				printf("Child task %d started\n", id);
				task(&identifier);
        exit(0);
	    }
		}
    identifier = 1;
    task(&identifier);
		printf("Parent waiting..\n");
		while ((wpid = wait(&status)) > 0);
    printf("\n");

out:
    return 0;
}

