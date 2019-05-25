#include <stdio.h>
#include <unistd.h>

int var = 0;

int do_nop()
{
    var++;
    asm("nop");
    var++;
    return 0;
}

int main()
{
    for (int i=0; i<100; i++)
      do_nop();
    return 0;
}
