/*

This file is licensed under the X11 license:

Copyright (C) 2013 Andreas Ehliar

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL ANDREAS EHLIAR BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Andreas Ehliar shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from Andreas Ehliar.

*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <sched.h>
#include <sys/types.h>
// General purpose error message
// A real system would probably have a better error handling method...
static void panic(char *message)
{
        fprintf(stderr,"Fatal error: %s\n", message);
        exit(1);
}


#define MAX_LOGENTRIES 200000
static unsigned int logindex;
static struct timespec timestamps[MAX_LOGENTRIES];
static void logtimestamp(void)
{
        clock_gettime(CLOCK_MONOTONIC, &timestamps[logindex++]);
        if(logindex > MAX_LOGENTRIES){
                logindex = 0;
        }
}

static void dumptimestamps(int unused)
{
        FILE *fp = fopen("/tmp/timestamps3.txt","w");
        int i;
        for(i=0; i < logindex; i++){
                if(timestamps[i].tv_sec > 0){
                        fprintf(fp,"%d.%09d\n", (int) timestamps[i].tv_sec,
                                (int) timestamps[i].tv_nsec);
                }
        }
        fclose(fp);
        exit(0);
}


// Initialize a GPIO pin in Linux using the sysfs interface
FILE *init_gpio(int gpioport)
{
        // Export the pin to the GPIO directory
        FILE *fp = fopen("/sys/class/gpio/export","w");
        fprintf(fp,"%d",gpioport);
        fclose(fp);

        // Set the pin as an output
        char filename[256];
        sprintf(filename,"/sys/class/gpio/gpio%d/direction",gpioport);
        fp = fopen(filename,"w");
        if(!fp){
                panic("Could not open gpio file");
        }
        fprintf(fp,"out");
        fclose(fp);

        // Open the value file and return a pointer to it.
        sprintf(filename,"/sys/class/gpio/gpio%d/value",gpioport);
        fp = fopen(filename,"w");
        if(!fp){
                panic("Could not open gpio file");
        }
        return fp;
}

// Given a FP in the stepper struct, set the I/O pin
// to the specified value. Uses the sysfs GPIO interface.
void setiopin(FILE *fp, int val)
{
        int x;
        x=0;
        fprintf(fp,"%d\n",val);
        fflush(fp);
        x=1;
}

// Adds "delay" nanoseconds to timespecs and sleeps until that time
static void sleep_until(struct timespec *ts, int delay)
{
        
        ts->tv_nsec += delay;
        if(ts->tv_nsec >= 1000*1000*1000){
                ts->tv_nsec -= 1000*1000*1000;
                ts->tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts,  NULL);
}

// Demo program for running two steppers at the same time connected to
// the Raspberry PI platform.
int main(int argc, char **argv)
{
        struct timespec ts;
        unsigned int delay = 1000*1000; // Note: Delay in ns
        unsigned int loops = atoi(argv[1]);
        FILE *pin0 = init_gpio(14);
        FILE *pin1 = init_gpio(15);
        FILE *pin2 = init_gpio(17);
        FILE *pin3 = init_gpio(18);

        cpu_set_t my_set;        /* Define your cpu_set bit mask. */
        CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
        CPU_SET(2, &my_set);     /* set the bit that represents core 7. */
        if (sched_setaffinity(0, sizeof(my_set), &my_set) == -1)
        {
            printf("err sched_setaffinity"); exit(1);
        }

        signal(SIGINT, dumptimestamps);
        clock_gettime(CLOCK_MONOTONIC, &ts);

        // Lock memory to ensure no swapping is done.
        if(mlockall(MCL_FUTURE|MCL_CURRENT)){
                fprintf(stderr,"WARNING: Failed to lock memory\n");
        }

        // Set our thread to real time priority
        struct sched_param sp;
        sp.sched_priority = 30;
        if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)){
                fprintf(stderr,"WARNING: Failed to set stepper thread"
                        "to real-time priority\n");
        }
        int probe;
        while(loops){
                sleep_until(&ts,delay);
                probe = 0;
                logtimestamp(); setiopin(pin0,1);
                sleep_until(&ts,delay); logtimestamp(); setiopin(pin3,0);
                sleep_until(&ts,delay); logtimestamp(); setiopin(pin1,1);
                sleep_until(&ts,delay); logtimestamp(); setiopin(pin0,0);
                probe = 1;
                sleep_until(&ts,delay); logtimestamp(); setiopin(pin2,1);
                sleep_until(&ts,delay); logtimestamp(); setiopin(pin1,0);
                sleep_until(&ts,delay); logtimestamp(); setiopin(pin3,1);
                sleep_until(&ts,delay); logtimestamp(); setiopin(pin2,0);
                loops--;
        }
}
