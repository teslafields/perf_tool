#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

#define sleep_ms 10
#define SIZE 1024*32
#define STACK
#define LOCK

int ITERATIONS;
int X = 4;

long long time_spent;
long long time_start;
long long time_stop;
long tout = sleep_ms*1000*1000;

unsigned short RUN = 1;
long factorial_arr[SIZE];

void nsleep(struct timespec deadline)
{
  	deadline.tv_nsec += tout;
  	if(deadline.tv_nsec >= 1000000000) {
  	    deadline.tv_nsec -= 1000000000;
  	    deadline.tv_sec++;
  	}
  	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
}

long factorial(int n)
{
    if (n == 0)
        return 1;
    else
        return(n * factorial(n-1));
}

void random_calc_task()
{
    factorial_arr[0] = 0;
    for (int i=1; i<SIZE; i++)
        factorial_arr[i] = factorial_arr[i-1] + i;
}

void *random_calc_thread(void *data)
{
    /* sleeping parameters */
    struct timespec start;
    while (ITERATIONS)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        random_calc_task();
        nsleep(start);
        ITERATIONS--;
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    if ( argc < 2 )
    {
        printf("Provide size, iterations, X and stack flag\n");
        goto out;
    }

    char *endptr;
    ITERATIONS = strtol(argv[1], &endptr, 10);
    
    printf("SIZE: %d ITE: %d X: %d \n", SIZE, ITERATIONS, X);
    
    pthread_attr_t attr;
    pthread_t tid;
    int ret;

    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) 
    {
        printf("init pthread attributes failed\n");
        goto out;
    }


#ifdef LOCK
        printf("Setting stack and mlock\n");
        /* Lock memory */
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            printf("mlockall failed: %m\n");
            exit(-2);
        }
#endif
#ifdef STACK
        /* Set a specific stack size  */
        ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN*2);
        if (ret) {
            printf("pthread setstacksize failed\n");
            goto out;
        }
#endif
    /* Create a pthread with specified attributes */
    ret = pthread_create(&tid, &attr, random_calc_thread, NULL);
    if (ret) {
        printf("create pthread failed: %d\n", ret);
        goto out;
    }
    /* Join the thread and wait until it is done */
    ret = pthread_join(tid, NULL);
    if (ret)
        printf("join pthread failed: %m\n");
out:
    return ret;

}
