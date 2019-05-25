// Note: Link with -lrt

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <assert.h>

#define sleep_ms 500

long long time_spent;
long long time_start;
long long time_stop;
long tout = sleep_ms*1000*1000;

char matrix[] = {
  0xA0, 0xA0, 0xA1, 0xA1, 0xA2, 0xA2, 0xA3, 0xA3, 0xA4, 0xA4, 0xA5, 0xA5, 0xA6, 0xA6, 0xA7, 0xA7, 0xA8, 0xA8, 0xA9, 0xA9,
  0xB0, 0xB0, 0xB1, 0xB1, 0xB2, 0xB2, 0xB3, 0xB3, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB7, 0xB7, 0xB8, 0xB8, 0xB9, 0xB9,
  0xC0, 0xE0, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 0x29, 0x29,
  0xD0, 0xF0, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35, 0x36, 0x36, 0x37, 0x37, 0x38, 0x38, 0x39, 0x39,
  0x40, 0x40, 0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 0x48, 0x48, 0x49, 0x49,
  0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x53, 0x53, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x57, 0x57, 0x58, 0x58, 0x59, 0x59,
  0x60, 0x60, 0x61, 0x61, 0x62, 0x62, 0x63, 0x63, 0x64, 0x64, 0x65, 0x65, 0x66, 0x66, 0x67, 0x67, 0x68, 0x68, 0x69, 0x69,
  0x70, 0x70, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x74, 0x74, 0x75, 0x75, 0x76, 0x76, 0x77, 0x77, 0x78, 0x78, 0x79, 0x79,
  0x80, 0x80, 0x81, 0x81, 0x82, 0x82, 0x83, 0x83, 0x84, 0x84, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87, 0x88, 0x88, 0x89, 0x89,
  0x90, 0x90, 0x91, 0x91, 0x92, 0x92, 0x93, 0x93, 0x94, 0x94, 0x95, 0x95, 0x96, 0x96, 0x97, 0x97, 0x98, 0x98, 0x99, 0x99
};


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

void produce_data(u_char *ptr, int i)
{
  int k;
  for(k=0; k<sizeof(matrix); k++)
  {
    matrix[k] = (char) (matrix[k] + i);
  }
  for (int i=0; i<sizeof(matrix); i++)
  {
      ptr[i] = (u_char) matrix[i];
  }
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Provide iterations value\n");
    return -1;
  }

  char *endptr;
  int ITERATIONS = strtol(argv[1], &endptr, 10);
  int ite=0;
  struct timespec start, stop;

	int oflags=O_RDWR | O_CREAT;
	int opt;

	off_t   length = 2 * 1024;
	char   *name   = "/malex.hello03.02";
	int     fd = shm_open(name, oflags, 0644 );

	ftruncate(fd, length);

	fprintf(stderr,"Shared Mem Descriptor: fd=%d\n", fd);

	assert (fd>0);

	u_char *ptr = (u_char *) mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	fprintf(stderr, "Shared Memory Address: %p [0..%lu]\n", ptr, length-1);
	fprintf(stderr, "Shared Memory Path: /dev/shm/%s\n", name );

	assert (ptr);

  while (ite < ITERATIONS)
  {
      /* get timestamp before */
      clock_gettime(CLOCK_MONOTONIC, &start);
      /* Do the task */
      produce_data(ptr, ite);
      /* get timestamp after */
      clock_gettime(CLOCK_MONOTONIC, &stop);

      time_start = (long long) start.tv_sec * 1000 * 1000 * 1000 + (long long) start.tv_nsec;
      time_stop  = (long long) stop.tv_sec * 1000 * 1000 * 1000 + (long long) stop.tv_nsec;
      time_spent = time_stop - time_start;

      if ( tout - time_spent > 0 )
      {
//          printf("%d took: %lld ms\n", ite, time_spent);
          nsleep(stop, tout - time_spent);
      }
      else
      {
          printf("Iteration %d took %lld (more than %d ms), skipping sleep..\n", ite, time_spent/1000, sleep_ms);
      }
      ite++;
  }

	close(fd);
	exit(0);
}