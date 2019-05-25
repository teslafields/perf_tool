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

long double *pi, w;

void do_paralelpi_task(int p)
{
  double x;
  long int i=0;
  pi[p] = 0.0;
  for (i=((interval/nr_thread)*p)+1; i<=(interval/nr_thread)*(p+1); i++)
  {
    x = w*(i - 0.5);
    pi[p] = pi[p] + 4.0/(1.0+(x*x));
  }
  i=0;
}

void* do_pi_thread(void *param)
{
	int p = *( (int *) param);
  do_paralelpi_task(p);
}

int main(int argc, char *argv[])
{
  if (argc > 4)
  {
    char *ptr;
    nr_thread = strtol(argv[1], &ptr, 10);
    synchro = strtol(argv[2], &ptr, 10);
    interval = strtol(argv[3], &ptr, 10);
    repeat = strtol(argv[4], &ptr, 10);
  }
  else
  {
    nr_thread = 4;
    synchro = 1;
    interval = 1000;
    repeat = 1;
  }
  count = nr_thread-1;
  pi = (long double*) malloc(nr_thread*sizeof(long double *));

  printf("PID main: %d, threads: %d, sync: %d, interval %d, repeat %d\n", getpid(), nr_thread, synchro, interval, repeat);
  w = 1.0/interval; 
	pi = (long double *) malloc(sizeof(long double) * nr_thread);

  pthread_t thread[nr_thread];
  int i, index[nr_thread];

  for (i=0; i<nr_thread; i++)
	{
    index[i] = i;
	}
  for (int k=0; k<repeat; k++)
  {
    for (i=nr_thread-1; i>=0; i--)
    {
      pthread_create(&thread[i], NULL, do_pi_thread, &index[i]);
    }
    i = 0;
    for (i=0; i<nr_thread; i++)
    {
       pthread_join(thread[i], NULL);
    }

    double Pi = 0.0;
    double sumpi = 0.0;
    for (int m=0; m<nr_thread; m++)
    {
      sumpi += pi[m];
    }
    //printf("Pi é aproximadamente %1.62Lf\n", Pi); //sumpi*w);
  }
  free(pi);
  return 0;
}
