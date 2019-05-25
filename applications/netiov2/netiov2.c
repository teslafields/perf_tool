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
#include <fcntl.h>
#include "evp_cript.h"

// #define DEBUG 1
#define ENCRYPT 1
#define NETWORK 1
#define sleep_ms 100

#define buf_size 10
#define msg_size 128
#define msg_len 4096

#define max_number 2048
#define min_number 129

int ITERATIONS;
int PORT;
char ADDR[20] = "10.10.100.3";

long long time_spent;
long long time_start;
long long time_stop;
long tout = sleep_ms*1000*1000;

/* A 256 bit key */
unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
/* A 128 bit IV */
unsigned char *iv = (unsigned char *)"0123456789012345";
unsigned char ciphertext[msg_len];
unsigned char buf_msg[buf_size][msg_len] = {0};

char send_buf[msg_size] = "{This is a mesage test. We hope it will me delivered with success!}";
char recv_buf[msg_len];
unsigned short buf_index = 0;
unsigned short DO_WRITE = 0;
unsigned short RUN = 1;
unsigned short writes_count = 0;

pthread_mutex_t lock;


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

void safe_write(FILE *afile)
{
    pthread_mutex_lock(&lock);
    int fd, n;
    char *filename = "apps/netiov2/cipherfile.out";
    // if (writes_count == 0)
        // afile = fopen(filename, "w");
    // else

    //afile = fopen(filename, "a+");
    fd = open(filename, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if ( !afile <  0 )
    {
        printf("Error open file\n");
    }
    else
    {
        for (int i=0; i<buf_size; i++)
        {
            //n = fprintf(afile, "%s\n", buf_msg[i]);
            n = write(fd, buf_msg[i], strlen(buf_msg[i]));
            if (n < 0)
                printf("Error write\n");
#ifdef LOGBYTES
            else
              printf("%d,", n);
#endif
        }


        /*
        int fd = fileno(afile);
        int ret = fsync(fd);
        if (ret < 0)
          printf("fsync error\n");
        */
        //fclose(afile);
        close(fd);
    }
    DO_WRITE = 0;
    writes_count++;
    pthread_mutex_unlock(&lock);
}

void netiov2_task(int sockfd, int ite)
{
    int n, i;
#ifdef NETWORK
    n = send(sockfd, send_buf, sizeof(send_buf), 0);
    if ( n < 0 )
    {
        printf("Send error: %d\n", n); exit(1);
    }
    n = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
    if ( n < 0 )
    {
        printf("recv error: %d\n", read); exit(1);
    }
#else
    memcpy(recv_buf, send_buf, sizeof(send_buf));
#endif

    // printf("[%d] %s\n", n, recv_buf);
#ifdef ENCRYPT
    int ciphertext_len = encrypt_now((unsigned char *) recv_buf, strlen(recv_buf),
      key, iv, ciphertext);
#else
    strncpy(ciphertext, recv_buf, msg_size);
#endif
    pthread_mutex_lock(&lock);
    /*
    int curlen = rand() % (max_number + 1 - min_number) + min_number;
    for (int k=0; k<curlen; k += msg_size)
    {
        for (int i=0; i<msg_size; i++)
            buf_msg[buf_index][k+i] = ciphertext[i];
    }
    */
    strncpy(buf_msg[buf_index], recv_buf, n-2);
    buf_msg[buf_index][n-1] = '\0';

    // printf("%ld: %s\n",strlen(buf_msg[buf_index]), buf_msg[buf_index]);
    if ((ite + 1)%buf_size == 0)
    {
        DO_WRITE = 1;
        buf_index = 0;
    }
    else
    {
        buf_index++;
    }
    pthread_mutex_unlock(&lock);
}

void set_thread_affinity(int cpu)
{
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
    CPU_SET(cpu, &my_set);     /* set the bit that represents core 7. */
    if (sched_setaffinity(0, sizeof(my_set), &my_set) == -1)
    {
        printf("err sched_setaffinity"); exit(1);
    }

}

void *netiov2_thread(void *data)
{
    /* sleeping parameters */
    struct timespec start, stop;
    int ite = 0;
    
    int sockfd;
    struct sockaddr_in servaddr;
    set_thread_affinity(2);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);

    inet_pton(AF_INET, ADDR, &(servaddr.sin_addr));
#ifdef NETWORK
    int conn = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if ( conn < 0 )
    {
        printf("could not connect: %s\n", strerror(errno));
    }
#endif
    printf("Starting iterations in netio_task\n");
    while (ite < ITERATIONS)
    {
        /* get timestamp before */
        clock_gettime(CLOCK_MONOTONIC, &start);
        /* Do the task */
        netiov2_task(sockfd, ite);
        /* get timestamp after */
        clock_gettime(CLOCK_MONOTONIC, &stop);

        time_start = (long long) start.tv_sec * 1000 * 1000 * 1000 + (long long) start.tv_nsec;
        time_stop  = (long long) stop.tv_sec * 1000 * 1000 * 1000 + (long long) stop.tv_nsec;
        time_spent = time_stop - time_start;

        if ( tout - time_spent > 0 )
        {
            nsleep(stop, tout - time_spent);
        }
        else
        {
            printf("Iteration %d took %lld (more than %d ms), skipping sleep..\n", ite, time_spent/1000, sleep_ms);
        }
        ite++;
    }
    RUN = 0;
    printf("%d writes\n", writes_count);
    return NULL;
}

void *write_thread(void *data)
{
    FILE *afile;
    set_thread_affinity(2);
    printf("Starting write_thread\n");
    while(RUN)
    {
        if(DO_WRITE)
        {
            safe_write(afile);
        }
        sched_yield();
    }
}

int main(int argc, char* argv[])
{
    if ( argc < 3 )
    {
        printf("Provide a port number and iterations\n");
        goto out;
    }

    char *endptr;
    PORT = strtol(argv[1], &endptr, 10);
    char *endptr2;
    ITERATIONS = strtol(argv[2], &endptr2, 10);

    printf("netiov2 pid: %ld\n", getpid());
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread[2];
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
#ifdef REALTIME_TASK
    /* Set scheduler policy and priority of pthread */
    printf("Setting thread to real-time task");
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
#endif
    /* Initialize Mutex */
    if(pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        goto out;
    }
    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread[0], &attr, netiov2_thread, NULL);
    if (ret) {
        printf("create pthread failed: %d\n", ret);
        goto out;
    }
    ret = pthread_create(&thread[1], &attr, write_thread, NULL);
    if (ret) {
        printf("create pthread failed: %d\n", ret);
        goto out;
    }

    /* Join the thread and wait until it is done */
    ret = pthread_join(thread[0], NULL);
    if (ret)
        printf("join pthread failed: %m\n");
    ret = pthread_join(thread[1], NULL);
    if (ret)
        printf("join pthread failed: %m\n");
out:
    return ret;

}
