#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <semaphore.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>


#define sleep_ms 10
#define buffer_size 2048
#define m_size (int) sqrt(buffer_size/2)
//#define DEBUG

int ITERATIONS;
long long time_spent;
long long time_start;
long long time_stop;
long tout = sleep_ms*1000*1000;


struct shared {
	sem_t mutex_p;
	sem_t mutex_c;
	char data[buffer_size];
} shared;

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

void print_data(char *data, int size)
{
  for(int i=0; i<size; i++)
		  printf("%02X%s", data[i], (i%25==24)?("\n"):(" ") );
  printf("\n");
}

void build_data(char *data, int ite)
{
	for (int i=0; i < buffer_size; i++)
	{
			data[i] = (char) (i + ite);
	}
}

void build_matrix(char *data, unsigned int size)
{
  int i, k=0, l=0;
  int tmp;

  int matrix[m_size][m_size];
  
  for (i=0; i<size; i+=2)
  {
    tmp = (int) data[i];
    tmp = tmp<<4;
    tmp += (int) data[i+1];

    if (l > m_size - 1)
    {
      l = 0;
      k++;
    }
    matrix[k][l] = tmp;
    l++;
  }
#ifdef DEBUG
  for (i=0; i<m_size; i++)
  {
    for (k=0; k<m_size; k++)
    {
      printf("%ld ", matrix[i][k]);
    }
    printf("\n");
  }
#endif
}

void shm_app_task(struct shared *ptr, int ite)
{
	build_data(ptr->data, ite);
  sem_post(&ptr->mutex_p);
  sem_wait(&ptr->mutex_c);
#ifdef DEBUG
  printf("IN  parent consumes data from child\n");
  print_data(ptr->data, buffer_size);
#endif
  build_matrix(ptr->data, buffer_size);
}

void child_task(struct shared *ptr)
{
  for (int i = 0; i < ITERATIONS; i++)
  {
    sem_wait(&ptr->mutex_p);
#ifdef DEBUG
    printf("IN  child consumes data from parent\n");
    print_data(ptr->data, buffer_size);
#endif
    build_data(ptr->data, 5);
    sem_post(&ptr->mutex_c);
  }
}

void parent_task(struct shared *ptr)
{
	struct timespec start, stop;
	int ite=0;
  while (ite < ITERATIONS)
  {
    /* get timestamp before */
    clock_gettime(CLOCK_MONOTONIC, &start);
    /* Do the task */
    shm_app_task(ptr, ite);
    /* get timestamp after */
    clock_gettime(CLOCK_MONOTONIC, &stop);

    time_start = (long long) start.tv_sec * 1000 * 1000 * 1000 + (long long) start.tv_nsec;
    time_stop  = (long long) stop.tv_sec * 1000 * 1000 * 1000 + (long long) stop.tv_nsec;
    time_spent = time_stop - time_start;

    if ( tout - time_spent > 0 )
    {
        // printf("%d took: %lld us\n", ite, time_spent/1000);
        nsleep(stop, tout - time_spent);
    }
    else
    {
        printf("Iteration %d took %lld (more than %d ms), skipping sleep..\n", ite, time_spent/1000, sleep_ms);
    }
    ite++;
  }
}

int main(int argc, char **argv) 
{
  if ( argc < 2 )
  {
    printf("Provide number of iterations\n");
    return -1;
  }

  char *endptr;
  ITERATIONS = strtol(argv[1], &endptr, 10);

	char *name = "/shm_app_fd01";
	int fd;
	struct shared *ptr;

  if ((fd = shm_open(name, O_RDWR | O_CREAT, S_IRWXU)) < 0) {
    perror("shm_open");
    exit(1);
  }

  if ( ftruncate(fd, sizeof(struct shared)) < 0 ) {
    perror("ftruncate");
    exit(1);
  }

  if ((ptr = (struct shared *) mmap(NULL, sizeof(struct shared), PROT_READ | PROT_WRITE,
      MAP_SHARED, fd, 0)) == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
	printf("Shared Mem Address: %p [0..%lu]\n", ptr, sizeof(struct shared));

  if (sem_init(&ptr->mutex_p, 1, 0) < 0) {
    perror("semaphore initialization");
    exit(1);
  }

  if (sem_init(&ptr->mutex_c, 1, 0) < 0) {
    perror("semaphore initialization");
    exit(1);
  }

	int childpid;
  if((childpid = fork()) == -1)
  {
    perror("fork");
    exit(1);
  }

  if(childpid == 0)
  {
    child_task(ptr);
  }
  else
  {
    parent_task(ptr);
		sem_destroy(&ptr->mutex_p);
		sem_destroy(&ptr->mutex_c);
    if (shm_unlink(name) < 0)
		{
      perror("shm_unlink error ");
      exit(EXIT_FAILURE);
    }
  }
	exit(0);
}
