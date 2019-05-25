//  Created by Emil Ernerfeldt on 2014-04-17.
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sched.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define sleep_ms 1000
#define nr_tasks 10

uint64_t ITERATIONS;
long long tout = sleep_ms*1000*1000;

typedef uint64_t Int;

typedef struct Node Node;

struct Node {
	Int payload; // ignored; just for plausability.
	Node* next;
};

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

void random_shuffle(Node** list, Int N)
{
	for (Int i=0; i<N-1; ++i) {
		Int swap_ix = i + rand() % (N-i);
		Node* tmp = list[swap_ix];
		list[swap_ix] = list[i];
		list[i] = tmp;
	}
}

// Returns nanoseconds per element.
double ram_bench_task(Int N, Int iters) {

	Int start = clock();
	Node* memory = (Node*)malloc(N * sizeof(Node));

	Node** nodes = (Node**)malloc(N * sizeof(Node*));
	Node** nodes2 = (Node**)malloc(N * sizeof(Node*));
	for (Int i=0; i<N; ++i) {
		nodes[i] = &memory[i];
	}
  memset(nodes2, 1, N*sizeof(Node*));
	//random_shuffle(nodes, N);

	// Link up the nodes:
	/*for (Int i=0; i<N-1; ++i) {
		nodes[i]->next = nodes[i+1];
	}
	nodes[N-1]->next = NULL;

	Node* start_node = nodes[0];
*/
	free(nodes); // Free up unused memory before meassuring:
	free(nodes2); // Free up unused memory before meassuring:

  /*
	for (Int it=0; it<iters; ++it) {
		Node* node = start_node;
		while (node) {
			node = node->next;
		}
	}
*/
	free(memory); // Free up unused memory before meassuring:
	Int dur = clock() - start;
	double ns = 1e9 * dur / CLOCKS_PER_SEC;
  ns = ns/1000;
	return ns; // / (N * iters);
}


void child_task()
{
	// Outputs data in gnuplot friendly .data format
	printf("#bytes    ns/elem\n");
	Int stopsPerFactor = 2; // For every power of 2, how many measurements do we do?
	Int minElemensFactor = 2;  // First measurement is 2^this number of elements.
	Int maxElemsFactor = 1000; // Last measurement is 2^this number of elements. 30 == 16GB of memory

  struct timespec start_t;
	Int min = stopsPerFactor * minElemensFactor;
	Int max = ITERATIONS; //stopsPerFactor * maxElemsFactor;
  
	for (Int ei=min; ei<=max; ++ei) {
		Int N = (((Int)floor(pow(2.0, (double) ei / stopsPerFactor) + 0.5)) % rand())% 4334937; //104729;
    N = N*10;
		Int reps = (Int)floor(1e9 / pow(N, 1.5) + 0.5);
		if (reps<1) reps = 1;
    reps = 1;

    Int start = clock();
    clock_gettime(CLOCK_MONOTONIC, &start_t);
		double ans = ram_bench_task(N, reps);
    Int stop = clock() - start;
    //nsleep(start_t);
    sched_yield();

    long long int ns = 1e6 * stop / CLOCKS_PER_SEC;
		printf("%llu\t\t%lld\t\t#(N=%llu,\treps=%llu)\t%llu/%llu\tsched_yield= %lld us\n", N*sizeof(Node), (long long int) ans, N, reps, ei-min+1, max-min+1, ns);
	}
}


int main(int argc, const char * argv[])
{
    int tasks = 0;
    if ( argc < 2 )
    {
        printf("Provide iteractions\n");
        goto out;
    }
    else if ( argc == 3 )
    {
        char *endptr;
        tasks = strtol(argv[2], &endptr, 10);
    }

    char *endptr2;
    ITERATIONS = strtol(argv[1], &endptr2, 10);

    pid_t   childpid, wpid;
		int status;

    if (!tasks)
      tasks = nr_tasks;

		for (int id=0; id<tasks; id++)
		{
    	if ((childpid = fork()) == 0)
			{
				printf("Child task %d started\n", id);
				child_task();
        printf("Child task %d exiting\n", id);
        exit(0);
	    }
		}
		printf("Parent waiting..\n");
		while ((wpid = wait(&status)) > 0);
    printf("\n");

out:
    return 0;
}
