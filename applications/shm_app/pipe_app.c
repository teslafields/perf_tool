#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define sleep_ms 10
#define buffer_size 2048
#define m_size (int) sqrt(buffer_size/2)

int ITERATIONS;

unsigned char childbuffer[buffer_size];
unsigned char parentbuffer[buffer_size];

long long time_spent;
long long time_start;
long long time_stop;
long tout = sleep_ms*1000*1000;


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

void pipe_app_task(int in, int out, int ite)
{
    build_data(parentbuffer, ite);

#ifdef DEBUG
	  printf("\n --------------------------------------- \n");
	  printf("\nParent write >>>>>>>>>>>>>>>>>>>>>>>>>\n");
	  print_data(parentbuffer, buffer_size);
#endif
		write(out, parentbuffer, buffer_size);

		int nbytes;
		nbytes = read(in, parentbuffer, buffer_size);

#ifdef DEBUG
		printf("Parent read <<<<<<<<<<<<<<<<<<<<<<<<<\n");
	  print_data(parentbuffer, buffer_size);
	  printf("\n --------------------------------------- \n");
#endif
		build_matrix(parentbuffer, buffer_size);
}


void child_task(int *parent_pipe, int *child_pipe)
{
		int in, out;
		int ite = 0;
		in = child_pipe[0];
		out = parent_pipe[1];
		int nbytes;
		while (ite < ITERATIONS)
		{
				nbytes = read(in, childbuffer, buffer_size);
#ifdef DEBUG
				printf("\nChild read: <<<<<<<<<<<<\n");
				print_data(childbuffer, buffer_size);
#endif
				build_data(childbuffer, 5);
#ifdef DEBUG
				printf("Child write: >>>>>>>>>>>\n");
				print_data(childbuffer, buffer_size);
#endif
				write(out, childbuffer, buffer_size);
				ite ++;
		}
		printf("Child out\n");
}

void parent_task(int *parent_pipe, int *child_pipe)
{
		int in, out;
		in = parent_pipe[0];
		out = child_pipe[1];

		struct timespec start, stop;

		int ite=0;
    while (ite < ITERATIONS)
    {
      /* get timestamp before */
      clock_gettime(CLOCK_MONOTONIC, &start);
      /* Do the task */
      pipe_app_task(in, out, ite);
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

int main(int argc, char* argv[])
{
    if ( argc < 2 )
    {
        printf("Provide number of iterations\n");
        return -1;
    }

    char *endptr;
    ITERATIONS = strtol(argv[1], &endptr, 10);

    int parent_pipe[2];
		int child_pipe[2];

    pid_t   childpid;

    if(pipe(parent_pipe) || pipe(child_pipe)) {
        perror("pipe(...)");
        exit(1);
    }

    if((childpid = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }

    if(childpid == 0)
    {
		    child_task(parent_pipe, child_pipe);
    }
    else
    {
        parent_task(parent_pipe, child_pipe);
    }
    return(0);
}

