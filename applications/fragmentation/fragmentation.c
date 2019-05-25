//  Created by Emil Ernerfeldt on 2014-04-17.
#include <unistd.h>
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

int ITERATIONS;
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
void fragmentation_task(Int N, Int iters) {

	Node* memory = (Node*)malloc(N * sizeof(Node));

	Node** nodes = (Node**)malloc(N * sizeof(Node*));
	for (Int i=0; i<N; ++i) {
		nodes[i] = &memory[i];
	}

	random_shuffle(nodes, N);

	// Link up the nodes:
	for (Int i=0; i<N-1; ++i) {
		nodes[i]->next = nodes[i+1];
	}
	nodes[N-1]->next = NULL;

	Node* start_node = nodes[0];

	free(nodes); // Free up unused memory before meassuring:


	for (Int it=0; it<iters; ++it) {
		Node* node = start_node;
		while (node) {
			node = node->next;
		}
	}

	free(memory); // Free up unused memory before meassuring:
}


void child_task()
{
	// Outputs data in gnuplot friendly .data format
	printf("#FRAGMENTATION START\nbytes    ns/elem\n");
	Int stopsPerFactor = 2; // For every power of 2, how many measurements do we do?
	Int minElemensFactor = 2;  // First measurement is 2^this number of elements.
	Int maxElemsFactor = 200; // Last measurement is 2^this number of elements. 30 == 16GB of memory

  struct timespec start_t;
	Int min = stopsPerFactor * minElemensFactor;
	Int max = ITERATIONS; //stopsPerFactor * maxElemsFactor;

  Int reps = (Int) 1e0;
  Int N = (Int) 1024*1024;

	for (Int ei=min; ei<=max; ++ei) {

    clock_gettime(CLOCK_MONOTONIC, &start_t);
		fragmentation_task(N, reps);
    nsleep(start_t);

	}
}


int main(int argc, const char * argv[])
{
    if ( argc < 2 )
    {
        printf("Provide iteractions\n");
        goto out;
    }

    char *endptr2;
    ITERATIONS = strtol(argv[1], &endptr2, 10);

    child_task();

out:
    return 0;
}
