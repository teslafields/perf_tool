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


#define MAX_STRING_SIZE (128)   /* we'll stop hashing after this many */
#define NUM_ITEMS 100
#define LOCK
#define ITERATIONS 100
#define SORT
#define sleep_ms 10

long hash_array[NUM_ITEMS];
char items[NUM_ITEMS][MAX_STRING_SIZE];
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

long factorial(int n)
{
    if (n == 0)
        return 1;
    else
        return(n * factorial(n-1));
}

unsigned long hash_function(const char *x, int size)
{
  int i;
  long y;
  y = factorial(size);
  unsigned long h = 0;
  int a = 31;
  int p = 117;
  for (i=0; i < size; i++)
  {
    h = (h*a + x[i]) % p;
  }
  return h;
}

void hash_task(int k)
{
  int i;
  for(i = 0; i < NUM_ITEMS; i++)
  {
    snprintf(items[i], MAX_STRING_SIZE, "%d%d value: %d code: %c%c%c%c", k, i, 2048*i+997*k, 50+i, 151+i, 25+i, 126+i);
    hash_array[i] = hash_function(items[i], sizeof(items[i]));
  }
#ifdef SORT
  int Sorted = 0;
  int Temp, Index;
  for (i = 0; i < NUM_ITEMS; i++)
  {
    Sorted = 1;
    for (Index = 0; Index <= NUM_ITEMS-1; Index++)
    {
      if (Index > NUM_ITEMS-i)
        break;
      if (hash_array[Index] > hash_array[Index + 1])
      {
        Temp = hash_array[Index];
        hash_array[Index] = hash_array[Index+1];
        hash_array[Index+1] = Temp;
        Sorted = 0;
      }
    }
    if (Sorted)
      break;
  }
#endif

}

void print_array()
{
  int i;
  for(i=0; i<NUM_ITEMS; i++)
  {
    printf("%ld ", hash_array[i]);
  }
  printf("\n");
}

void *hash_thread(void *data)
{
  printf("Starting thread\n");
  /* sleeping parameters */
  struct timespec start;
  int i, ite = 0;

  for(i = 0; i < ITERATIONS; i++)
  {
    /* get timestamp before */
    clock_gettime(CLOCK_MONOTONIC, &start);
    /* Do the task */
    hash_task(i);
    nsleep(start);
  }
  return NULL;
}

int main(int argc, char* argv[])
{
  if (argc < 2)
    goto out;

  char *ptr;
  int lock = strtol(argv[1], &ptr, 10);
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  int ret;
  ret = pthread_attr_init(&attr);
  if (ret) {
    printf("init pthread attributes failed\n");
    goto out;
  }
#ifdef LOCK
  printf("iINFO: locking...\n");
  /* Lock memory */
  if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
    printf("mlockall failed: %m\n");
    exit(-2);
  }
  /* Initialize pthread attributes (default values) */

  /* Set a specific stack size  */
  ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN + 0x1000);
  if (ret) {
    printf("pthread setstacksize failed\n");
    goto out;
  }
#endif
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
  ret = pthread_create(&thread, &attr, hash_thread, NULL);
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
/* implements universal hashing using random bit-vectors in x */
/* assumes number of elements in x is at least BITS_PER_CHAR * MAX_STRING_SIZE */
