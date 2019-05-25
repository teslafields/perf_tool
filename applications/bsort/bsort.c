#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <limits.h>
#include <string.h>

#define ITERATIONS 100000
#define FALSE 0
#define TRUE 1
#define NUMELEMS 100
#define sleep_ms 10

long long  buf[ITERATIONS];
long long  buf_start[ITERATIONS];
long long  buf_stop[ITERATIONS];
long tout = sleep_ms*1000*1000;

int Array_bckp[NUMELEMS] = {
    100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75,
    74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50,
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25,
    24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
};

void handle_file()
{
    int i;
    FILE* afile;
    afile = fopen("apps/bsort/bsort_clock_out", "w");
    if ( !afile )
    {
        printf("Error open file\n");
    }
    else
    {
        for (i=0; i<ITERATIONS; i++)
        {
	         fprintf(afile, "%lld\n", buf[i]);
        }
        fclose(afile);
    }
}

void nsleep(struct timespec deadline, long long int ns)
{
    // Add the time you want to sleep
  	deadline.tv_nsec += ns;
  	// Normalize the time to account for the second boundary
  	if(deadline.tv_nsec >= 1000000000) {
  	    deadline.tv_nsec -= 1000000000;
  	    deadline.tv_sec++;
  	}
  	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
}

void bsort_task(int *Array)
{
    /* start sorting */
    int Sorted = FALSE;
    int Temp, Index, i;
    for (i = 1; i <= NUMELEMS-1; i++)
    {
        Sorted = TRUE;
        for (Index = 1; Index <= NUMELEMS-1; Index ++)
        {
            if (Index > NUMELEMS-i)
                break;
            if (Array[Index] > Array[Index + 1])
            {
                Temp = Array[Index];
                Array[Index] = Array[Index+1];
                Array[Index+1] = Temp;
                Sorted = FALSE;
            }
        }
        if (Sorted)
            break;
    }
}

void *bsort_thread(void *data)
{
    printf("Starting rt_bsort with nanosleep\n");
    /* sleeping parameters */
    struct timespec start, stop;
    long start_bef = 0;
    int ite = 0;
    int * Array = malloc(NUMELEMS * sizeof(int));

    /* iterate for 10 000 times */
    while (ite < ITERATIONS)
    {
        memcpy(Array, Array_bckp, NUMELEMS * sizeof(int));
        /* get timestamp before */
        clock_gettime(CLOCK_MONOTONIC, &start);
        /* Do the task */
        bsort_task(Array);
        /* get timestamp after */
        clock_gettime(CLOCK_MONOTONIC, &stop);
        buf_start[ite] = (long long) start.tv_sec * 1000 * 1000 * 1000 + (long long) start.tv_nsec;
        buf_stop[ite] = (long long) stop.tv_sec * 1000 * 1000 * 1000 + (long long) stop.tv_nsec;
        buf[ite] = buf_stop[ite] - buf_start[ite];

        if ( tout - buf[ite] > 0 )
        {
            nsleep(stop, tout - buf[ite]);
        }
        else
        {
            printf("Ite %d took %lld (more than %d ms)\n", ite, buf[ite]/1000000, sleep_ms);
        }
        start_bef = buf_start[ite];
        ite++;
    }
    handle_file();
    free(Array);
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
    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread, &attr, bsort_thread, NULL);
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
