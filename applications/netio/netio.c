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
#include "evp_cript.h"

#include <sys/stat.h>
#include <fcntl.h>


// #define DEBUG 1
#define ENCRYPT
#define NETWORK
#define sleep_ms 10
//#define LOCK
#define msg_size 128

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
unsigned char ciphertext[msg_size];
unsigned char buf_msg[msg_size];

char send_buf[4] = {0x21, 0x04, 0x10, 0x00};
char recv_buf[msg_size];

void handle_file(char *filename)
{
    int i;
    FILE* afile;
    afile = fopen(filename, "w");
    if ( !afile )
    {
        printf("Error open file\n");
    }
    else
    {
        fprintf(afile, "%s\n", buf_msg);
        fclose(afile);
    }
}

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

void netio_task(int sockfd, FILE *afile, int ite)
{
    int n, i, count;
    count = 1;
#ifdef NETWORK
    n = send(sockfd, send_buf, sizeof(send_buf), 0);
    count++;
    if ( n < 0 )
    {
        printf("Send error: %d\n", n); exit(1);
    }
    count++;
    n = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
    if ( n < 0 )
    {
        printf("recv error: %d\n", read); exit(1);
    }
#else
    memcpy(recv_buf, send_buf, sizeof(send_buf));
#endif
    count++;
    int ciphertext_len;
#ifdef ENCRYPT
		count++;
    ciphertext_len = encrypt_now((unsigned char *) recv_buf, sizeof(recv_buf),
      key, iv, ciphertext);
    count++;
#else
    strcpy(ciphertext, recv_buf);
    ciphertext_len = strlen(ciphertext);
#endif
    count++;
    char *filename = "cipherfile.out";

    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXG);
    if (fd < 0)
        printf("ERR FD\n");
    else
    {
      write(fd, ciphertext, sizeof(ciphertext));
      close(fd);
    }
/*
    afile = fopen(filename, "w");
    if ( !afile )
    {
        printf("Error open file\n");
    }
    else
    {
        fprintf(afile, "%s\n", ciphertext);
        fclose(afile);
    }
*/
    count++;
}

void *netio_thread(void *data)
{
    /* sleeping parameters */
    struct timespec start, stop;
    int ite = 0;
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    //CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
    //CPU_SET(2, &my_set);     /* set the bit that represents core 7. */
    //if (sched_setaffinity(0, sizeof(my_set), &my_set) == -1)
    //{
    //    printf("err sched_setaffinity"); exit(1);
    //}

    FILE *afile;
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    inet_pton(AF_INET, ADDR, &(servaddr.sin_addr));

    printf("Thread PID: %ld connecting at %s:%d\n", getpid(), ADDR, PORT);

    int conn = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if ( conn < 0 )
    {
        printf("could not connect: %s\n", strerror(errno));
    }
    else
    {
        printf("Successfuly connected to %s:%d\n", ADDR, PORT);
        while (ite < ITERATIONS)
        {
            /* get timestamp before */
            clock_gettime(CLOCK_MONOTONIC, &start);
            /* Do the task */
            netio_task(sockfd, afile, ite);
            /* get timestamp after */
            clock_gettime(CLOCK_MONOTONIC, &stop);

            nsleep(start);
            ite++;
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    if ( argc < 3 )
    {
        printf("Provide a port number and iteractions\n");
        goto out;
    }
    char *endptr;
    PORT = strtol(argv[1], &endptr, 10);
    char *endptr2;
    ITERATIONS = strtol(argv[2], &endptr2, 10);

    printf("netio pid: %ld\n", getpid());
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;

    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        goto out;
    }

#ifdef LOCK
    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }
    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret) {
        printf("pthread setstacksize failed\n");
        goto out;
    }
#endif
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
    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread, &attr, netio_thread, NULL);
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
