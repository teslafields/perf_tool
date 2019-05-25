#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/types.h>
#include <semaphore.h>

//#define PEVENT
int nr_thread;
int interval;
int repeat;
int synchro;
int count;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;

long double *pi, w;
volatile double sh_pi = 0.0;


int mymutex(int p)
{

    int x = p;
}

void do_pi_task(int p)
{
    pthread_barrier_wait(&barrier);
    double x;
    long int i=0;
    pi[p] = 0.0;
    for (i=((interval/nr_thread)*p)+1; i<=(interval/nr_thread)*(p+1); i++)
    {
        x = w*(i - 0.5);
        pthread_mutex_lock(&mutex);
        sh_pi = sh_pi + 4.0/(1.0+(x*x));
        pthread_mutex_unlock(&mutex);
    }
    i = 0;
}

void* do_pi_thread(void *param)
{
    int p = *( (int *) param);
    do_pi_task(p);
}

int main(int argc, char *argv[])
{
    int prio;
    if (argc > 5)
    {
        char *ptr;
        nr_thread = strtol(argv[1], &ptr, 10);
        synchro = strtol(argv[2], &ptr, 10);
        interval = strtol(argv[3], &ptr, 10);
        repeat = strtol(argv[4], &ptr, 10);
        prio = strtol(argv[5], &ptr, 10);
    }
    else
    {
        nr_thread = 4;
        synchro = 1;
        interval = 1000;
        repeat = 1;
        prio = 5;
    }
    count = nr_thread-1;
    pi = (long double*) malloc(nr_thread*sizeof(long double *));

    printf("PID main: %d, threads: %d, sync: %d, interval %d, repeat %d, prio %d\n", getpid(), nr_thread, synchro, interval, repeat, prio);
    w = 1.0/interval; 
    pi = (long double *) malloc(sizeof(long double) * nr_thread);

    pthread_t thread[nr_thread];
    if (pthread_barrier_init(&barrier, NULL, nr_thread) != 0)
        printf("barrier error\n");
    pthread_attr_t attr;
    struct sched_param param;
    int i, index[nr_thread];
    int ret;
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        return 0;
    }
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret) {
        printf("pthread setstacksize failed\n");
        return 0;
    }
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        printf("pthread setschedpolicy failed\n");
        return 0;
    }
    param.sched_priority = 98;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        printf("pthread setschedparam failed\n");
        return 0;
    }
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        printf("pthread setinheritsched failed\n");
        return 0;
    }
    for (i=0; i<nr_thread; i++)
    {
        index[i] = i;
    }
    for (int k=0; k<repeat; k++)
    {
        for (i=nr_thread-1; i>=0; i--)
        {
            if (i == prio)
                pthread_create(&thread[i], &attr, do_pi_thread, &index[i]);
            else
                pthread_create(&thread[i], NULL, do_pi_thread, &index[i]);
        }
        i = 0;
        for (i=0; i<nr_thread; i++)
        {
            pthread_join(thread[i], NULL);
        }

        double Pi = 0.0;
        double sumpi = 0.0;
        Pi = sh_pi*w;
        for (int m=0; m<nr_thread; m++)
        {
            sumpi += pi[m];
        }
        printf("Pi é aproximadamente %1.62Lf\n", Pi); //sumpi*w);
        sh_pi = 0.0;
    }
    free(pi);
    pthread_barrier_destroy(&barrier);
    return 0;
}
