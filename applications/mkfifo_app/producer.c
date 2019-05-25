#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#define buffer_size 2048

char buff[buffer_size];

void build_data(char *data, int ite)
{

		for (int i=0; i < buffer_size; i++)
		{
				data[i] = (char) (i + ite);
		}
}

int main()
{
    int fd, k, total=10000;
    char * myfifo = "/tmp/myfifo";

    /* create the FIFO (named pipe) */
    mkfifo(myfifo, 0666);

    /* write "Hi" to the FIFO */
    fd = open(myfifo, O_WRONLY);
		
		int nbytes;
		while (k < total)
		{
				nbytes = read(fd, buff, buffer_size);

				build_data(buff, 5);

    		write(fd, buff, buffer_size);
				k+=1;
		}
    close(fd);
    /* remove the FIFO */
    unlink(myfifo);

    return 0;
}
