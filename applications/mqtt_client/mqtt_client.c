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

#include <MQTTClient.h>

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientPub"
#define CLIENTID2   "ExampleClientSub"
#define TOPIC       "mqtt/topic/test"
#define QOS         1
#define TIMEOUT     10000L

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


int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;
    printf("<-- %s [%d] %s\n", topicName, message->payloadlen, message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}


void *mqtt_client_thread(void *data)
{
    struct timespec start, stop;
    for (int i=0; i<MSGLEN; i++)
        send_buf[i] = 0x31;
    send_buf[MSGLEN-1] = 0;

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);

    conn_opts.keepAliveInterval = 30;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }

    pubmsg.payload = send_buf;
    pubmsg.payloadlen = strlen(send_buf);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    
    printf("Thread PID: %ld\n", getpid());
    while (ite < ITERATIONS) {
        /* get timestamp before */
        clock_gettime(CLOCK_MONOTONIC, &start);
        /* Do the task */
        MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        /* get timestamp after */
        clock_gettime(CLOCK_MONOTONIC, &stop);

        nsleep(start);
        ite++;
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

void *mqtt_sub_thread(void *data)
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;
    MQTTClient_create(&client, ADDRESS, CLIENTID2,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    MQTTClient_setCallbacks(client, NULL, NULL, msgarrvd, NULL);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\n", TOPIC);
    MQTTClient_subscribe(client, TOPIC, QOS);
    while (ite < ITERATIONS)
    {
        sleep(1);
    }
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

int main(int argc, char* argv[])
{
    if ( argc < 2 )
    {
        printf("Provide iteractions\n");
        goto out;
    }
    ITERATIONS = strtol(argv[1], NULL, 10);

    printf("mqtt_client pid: %ld - iterations: %d\n", getpid(), ITERATIONS);
    
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
    ret = pthread_create(&thread2, NULL, mqtt_sub_thread, NULL);
    if (ret) {
        printf("tcp server: create pthread failed: %d\n", ret);
        goto out;
    }
    ret = pthread_create(&thread, &attr, mqtt_client_thread, NULL);
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
