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


#define ITERATIONS 100
#define sleep_ms 50
#define FFT_SIZE 8192
#define PI 3.14159
#define M_PI 3.14159

double ar[FFT_SIZE];
double ai[FFT_SIZE] = {0.,  };

long long  buf[ITERATIONS];
long long  buf_start[ITERATIONS];
long long  buf_stop[ITERATIONS];
long tout = sleep_ms*1000*1000;


void handle_file()
{
    int i;
    FILE* afile;
    afile = fopen("apps/fft/fft_clock_out", "w");
    if ( !afile )
    {
        printf("Error open file\n");
    }
    else
    {
        for (i=0; i<ITERATIONS; i++)
        {
	         fprintf(afile, "%lld\n", buf[i]/1000);
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

static double fabs(double n)
{
  double f;

  if (n >= 0) f = n;
  else f = -n;
  return f;
}

static double log(double n)
{
    return(4.5);
}

static double sin(rad)
double rad;
{
  double app;

  double diff;
  int inc = 1;

  while (rad > 2*PI)
	rad -= 2*PI;
  while (rad < -2*PI)
    rad += 2*PI;
  app = diff = rad;
   diff = (diff * (-(rad*rad))) /
      ((2.0 * inc) * (2.0 * inc + 1.0));
    app = app + diff;
    inc++;
  while(fabs(diff) >= 0.00001) {
    diff = (diff * (-(rad*rad))) /
      ((2.0 * inc) * (2.0 * inc + 1.0));
    app = app + diff;
    inc++;
  }

  return(app);
}

static double cos(double rad)
{
  double sin();
  return (sin (PI / 2.0 - rad));
}

int fft_task(int n, int flag)
{

	 int i, j, k, it, xp, xp2, j1, j2, iter;
	 double sign, w, wr, wi, dr1, dr2, di1, di2, tr, ti, arg;

   for(i = 0; i < n; i++)
 	  ar[i] = cos(2*M_PI*i/n);

	 if(n < 2) return(999);
	 iter = log((double)n)/log(2.0);
	 j = 1;
#ifdef DEBUG
	printf("iter=%d\n",iter);
#endif
	 for(i = 0; i < iter; i++)
	   j *= 2;
	 if(fabs(n-j) > 1.0e-6)
	   return(1);

	 /*  Main FFT Loops  */
	 sign = ((flag == 1) ? 1.0 : -1.0);
	 xp2 = n;
	 for(it = 0; it < iter; it++)
	 {
			 xp = xp2;
			 xp2 /= 2;
			 w = PI / xp2;
#ifdef DEBUG
	printf("xp2=%d\n",xp2);
#endif
			 for(k = 0; k < xp2; k++)
			 {
					 arg = k * w;
					 wr = cos(arg);
					 wi = sign * sin(arg);
					 i = k - xp;
					 for(j = xp; j <= n; j += xp)
					 {
							 j1 = j + i;
							 j2 = j1 + xp2;
							 dr1 = ar[j1];
							 dr2 = ar[j2];
							 di1 = ai[j1];
							 di2 = ai[j2];
							 tr = dr1 - dr2;
							 ti = di1 - di2;
							 ar[j1] = dr1 + dr2;
							 ai[j1] = di1 + di2;
							 ar[j2] = tr * wr - ti * wi;
							 ai[j2] = ti * wr + tr * wi;
					 }
			 }
	 }
	 /*  Digit Reverse Counter  */
	 j1 = n / 2;
	 j2 = n - 1;
	 j = 1;
#ifdef DEBUG
	printf("j2=%d\n",j2);
#endif
	 for(i = 1; i <= j2; i++)
	 {
			 if(i < j)
			 {
					tr = ar[j-1];
					ti = ai[j-1];
					ar[j-1] = ar[i-1];
					ai[j-1] = ai[i-1];
					ar[i-1] = tr;
					ai[i-1] = ti;
			 }
			 k = j1;
			 while(k < j)
			 {
					j -= k;
					k /= 2;
			 }
			 j += k;
	 }
	 if(flag == 0) return(0);
	 w = n;
	 for(i = 0; i < n; i++)
	 {
			 ar[i] /= w;
			 ai[i] /= w;
	 }
	 return(0);
}

void *fft_thread(void *data)
{
    printf("Starting thread\n");
    /* sleeping parameters */
    struct timespec start, stop;
    int i, ite = 0;
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
    CPU_SET(2, &my_set);     /* set the bit that represents core 7. */
    if (sched_setaffinity(0, sizeof(my_set), &my_set) == -1)
    {
        printf("err sched_setaffinity"); exit(1);
    }

    for(i = 0; i < ITERATIONS; i++)
    {
        /* get timestamp before */
        clock_gettime(CLOCK_MONOTONIC, &start);
        /* Do the task */
        fft_task(FFT_SIZE, 0);
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
        ite++;
    }

    handle_file();
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
    ret = pthread_create(&thread, &attr, fft_thread, NULL);
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
