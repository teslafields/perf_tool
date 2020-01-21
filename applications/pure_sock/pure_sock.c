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

#include <sys/stat.h>
#include <fcntl.h>


#define DEBUG 1
#define NETWORK
#define sleep_ms 10
#define MSGLEN 180
#define PORT 6000

int ITERATIONS;
int ite=0;
char ADDR[20] = "127.0.0.1";

long long time_spent;
long long time_start;
long long time_stop;
long tout = sleep_ms*1000*1000;

unsigned char buf_msg[MSGLEN];

char send_buf[MSGLEN]; //{0x11, 0x01, 0x12, 0x8, 0x9, 0x3, 0x34, 0x8d, 0xc4, 0x9b, 0xff, 0x21, 0x04, 0x10, 0x00};

void nsleep(struct timespec deadline)
{
  	deadline.tv_nsec += tout;
  	if(deadline.tv_nsec >= 1000000000) {
  	    deadline.tv_nsec -= 1000000000;
  	    deadline.tv_sec++;
  	}
  	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
}

void pure_sock_task(int sockfd)
{
    int n, i, count;
    count = 1; 
    n = send(sockfd, send_buf, sizeof(send_buf), 0);
    if ( n < 0 ) {
        printf("Send error: %d\n", n); exit(1);
    }
}

void *pure_sock_thread(void *data)
{
    struct timespec start, stop;
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    inet_pton(AF_INET, ADDR, &(servaddr.sin_addr));

    for (int i=0; i<MSGLEN; i++)
        send_buf[i] = 0x31;

    clock_gettime(CLOCK_MONOTONIC, &start);
    nsleep(start);

    int conn = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if ( conn < 0 ) {
        printf("could not connect: %s\n", strerror(errno));
        exit(1);
    }

    printf("Thread PID: %ld\n", getpid());
    while (ite < ITERATIONS) {
        /* get timestamp before */
        clock_gettime(CLOCK_MONOTONIC, &start);
        /* Do the task */
        pure_sock_task(sockfd);
        /* get timestamp after */
        clock_gettime(CLOCK_MONOTONIC, &stop);

        nsleep(start);
        ite++;
    }
    close(sockfd);
    return NULL;
}

void *tcp_server_thread(void *data)
{
    int server_fd, client_fd, err;
    struct sockaddr_in server, client;
    char buf[MSGLEN];
  
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("Could not create socket\n"); exit(1);
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
  
    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);
  
    err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
    if (err < 0) {
        printf("Could not bind socket\n"); exit(1);
    }
  
    err = listen(server_fd, 128);
    if (err < 0) {
        printf("Could not listen on socket\n"); exit(1);
    }

    printf("Server is listening on %s:%d\n", INADDR_ANY, PORT);
    while (1) {
      if (ite >= ITERATIONS-1)
          break;
      printf("Waiting client..\n");
      socklen_t client_len = sizeof(client);
      client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);
      printf("New client connected %d:%d\n", client.sin_addr.s_addr, client.sin_port);
      if (client_fd < 0) {
            printf("Could not establish new connection\n"); exit(1);
      }
      while (1) {
        int n = recv(client_fd, buf, MSGLEN, 0);
        buf[MSGLEN-1] = 0;

        if (n <= 0) {
            printf("done reading\n");
            break;
        }
#ifdef DEBUG
        printf("%d <- [%d] %s\n", ite, n, buf);
#endif
    }
  }

}

int main(int argc, char* argv[])
{
    if ( argc < 2 )
    {
        printf("Provide a port number and iteractions\n");
        goto out;
    }
    ITERATIONS = strtol(argv[1], NULL, 10);

    printf("pure_sock pid: %ld - iterations: %d\n", getpid(), ITERATIONS);
    
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread, thread2;
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
    ret = pthread_create(&thread2, NULL, tcp_server_thread, NULL);
    if (ret) {
        printf("tcp server: create pthread failed: %d\n", ret);
        goto out;
    }
    ret = pthread_create(&thread, &attr, pure_sock_thread, NULL);
    if (ret) {
        printf("create pthread failed: %d\n", ret);
        goto out;
    }
    ret = pthread_join(thread2, NULL);
    if (ret)
        printf("join pthread failed: %m\n");
    ret = pthread_join(thread, NULL);
    if (ret)
        printf("join pthread failed: %m\n");

out:
    return ret;

}
