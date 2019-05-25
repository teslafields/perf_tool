#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <math.h>

//char *socket_path = "./socket";
char *socket_path = "\0hidden";

long long determinant(int a[10][10], int k)
{
  int s=1,b[10][10];
	long long det=0;
  int i,j,m,n,c;
  if (k==1)
  {
     return (a[0][0]);
  }
  else
  {
     det=0;
     for (c=0;c<k;c++)
     {
        m=0;
        n=0;
        for (i=0;i<k;i++)
        {
            for (j=0;j<k;j++)
            {
                b[i][j]=0;
                if (i != 0 && j != c)
                {
                   b[m][n]=a[i][j];
                   if (n<(k-2))
                     n++;
                   else
                   {
                     n=0;
                     m++;
                   }
                }
             }
          }
          det=det + s * (a[0][c] * determinant(b,k-1));
          s=-1 * s;
          }
    }
    return (det);
}

void build_matrix(char *data, unsigned int size)
{
  int i, k=0, l=0;
  int tmp;
  int m_size = size/20;
  int matrix[m_size][m_size];
  for (i=0; i<size; i+=2)
  {
    tmp = (int) data[i];
    tmp = tmp<<4;
    tmp += (int) data[i+1];

    if (l > m_size - 1)
    {
      l = 0;
      k++;
    }
    matrix[k][l] = tmp;
    l++;
  }
  for (i=0; i<m_size; i++)
  {
    for (k=0; k<m_size; k++)
    {
      printf("%ld ", matrix[i][k]);
    }
    printf("\n");
  }
  printf("\n");
  printf("Determinant: %lld\n", determinant(matrix, m_size));
}

int main(int argc, char *argv[]) {
  struct sockaddr_un addr;
  char buf[200];
  int fd,cl,rc;

  if (argc > 1) socket_path=argv[1];

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  if (*socket_path == '\0') {
    *addr.sun_path = '\0';
    strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
  } else {
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    unlink(socket_path);
  }

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    exit(-1);
  }

  if (listen(fd, 5) == -1) {
    perror("listen error");
    exit(-1);
  }

  while (1) {
    if ( (cl = accept(fd, NULL, NULL)) == -1) {
      perror("accept error");
      continue;
    }

    while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
      printf("read %u bytes: %.*s\n", rc, rc, buf);
      build_matrix(buf, rc);
			break;
    }

		break;
    if (rc == -1) {
      perror("read");
      exit(-1);
    }
    else if (rc == 0) {
      printf("EOF\n");
      close(cl);
    }
  }


  return 0;
}
