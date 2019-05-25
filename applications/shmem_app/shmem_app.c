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

#define sleep_ms 100

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

long long determinant(int a[10][10], int k)
{
  int s=1,b[10][10];
	long long det=0;
  int i,j,m,n,c;
  if (k==1)
  {
     return (a[0][0]);
  }
  else
  {
     det=0;
     for (c=0;c<k;c++)
     {
        m=0;
        n=0;
        for (i=0;i<k;i++)
        {
            for (j=0;j<k;j++)
            {
                b[i][j]=0;
                if (i != 0 && j != c)
                {
                   b[m][n]=a[i][j];
                   if (n<(k-2))
                     n++;
                   else
                   {
                     n=0;
                     m++;
                   }
                }
             }
          }
          det=det + s * (a[0][c] * determinant(b,k-1));
          s=-1 * s;
          }
    }
    return (det);
}

void build_matrix(char *data, unsigned int size)
{
  int i, k=0, l=0;
  int tmp;
  int m_size = size/20;
  int matrix[m_size][m_size];
  
  printf("\n\n");

  for(i=0; i<200; i++)
		fprintf(stdout, "%02X%s", data[i], (i%25==24)?("\n"):(" ") );

  printf("\n\n");
  /*
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
  for (i=0; i<m_size; i++)
  {
    for (k=0; k<m_size; k++)
    {
      printf("%ld ", matrix[i][k]);
    }
    printf("\n");
  }*/
  //printf("Determinant: %lld\n", determinant(matrix, m_size));
}

int main(int argc, char **argv) {

  int ite = 0;
  int ITERATIONS = 100;
  struct timespec start, stop;

	int oflags=O_RDWR;
	int i;

	char   *name   = "/malex.hello03.02";
	int     fd     = shm_open(name, oflags, 0644 );

	fprintf(stderr,"Shared Mem Descriptor: fd=%d\n", fd);

	assert (fd>0);

	struct stat sb;

	fstat(fd, &sb);
	off_t length = sb.st_size ;

	u_char *ptr = (u_char *) mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	fprintf(stderr, "Shared Mem Address: %p [0..%lu]\n", ptr, length-1);
	assert (ptr);

  while (ite < ITERATIONS)
  {
      /* get timestamp before */
      clock_gettime(CLOCK_MONOTONIC, &start);
      /* Do the task */
      build_matrix((char *) ptr, 200);
      /* get timestamp after */
      clock_gettime(CLOCK_MONOTONIC, &stop);

      time_start = (long long) start.tv_sec * 1000 * 1000 * 1000 + (long long) start.tv_nsec;
      time_stop  = (long long) stop.tv_sec * 1000 * 1000 * 1000 + (long long) stop.tv_nsec;
      time_spent = time_stop - time_start;

      if ( tout - time_spent > 0 )
      {
          printf("%d took: %lld ms\n", ite, time_spent);
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
