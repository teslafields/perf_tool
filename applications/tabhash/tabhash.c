#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h> /* define inteiros de tamanho específico */

#define 	TAMHASH		113

#pragma pack(push, 1)

struct bmpHeader {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1,
            reserved2;
    uint32_t offset,
            header_size;
    int32_t width,
        height;    
    uint16_t planes, 
            bits;
    uint32_t compression,
                imagesize;
    int32_t xresolution,
        yresolution;
    uint32_t ncolours,
                importantcolours;
    uint32_t redbitmask,greenbitmask,bluebitmask,alphabitmask;
    uint32_t ColorSpaceType;
    uint32_t ColorSpaceEndPoints[9];
    uint32_t Gamma_Red,Gamma_Green,Gamma_Blue,intent,ICCProfileData,ICCProfileSize,Reserved;
};

#pragma pack(pop)

struct imgData{
  long mtype;
  char name[255];
  struct bmpHeader bH;
};

struct registro{
  struct imgData data;
  char nome[255];
	struct registro *prox;
};


struct registro *tabela[TAMHASH];


void printbin(char texto[], unsigned int valor)
{
     int i;
     printf("%s",texto);
     for (i=31;i>=0;i--) printf("%d",valor >> i & 1); 
     printf("\n");
}


void print_data(struct imgData *ptr)
{
    printf("Msg Type = %d\n", ptr->mtype);
    printf("Image Name = %s\n", ptr->name);
    printf("Image size = %d x %d\n", ptr->bH.width, ptr->bH.height);
    printf("Tamanho do arquivo = %d\n", ptr->bH.size);
    printf("Number of colour planes is %hu\n", ptr->bH.planes);
    printf("Bits per pixel is %hu\n", ptr->bH.bits);
    printf("Compression type is %u\n", ptr->bH.compression);
    printf("Number of colours is %u\n", ptr->bH.ncolours);
    printf("Number of required colours is %u\n", ptr->bH.importantcolours);
    printf("X resolution: %d - Y resolution: %d (ps: pixels per meter)\n", ptr->bH.xresolution, ptr->bH.yresolution);
    printf("Header size is %d\n", ptr->bH.header_size);
    printbin("Red channel bitmask ", ptr->bH.redbitmask);
    printbin("Blue channel bitmask ", ptr->bH.bluebitmask);
    printbin("Green channel bitmask ", ptr->bH.greenbitmask);
    printbin("Alpha channel bitmask ", ptr->bH.alphabitmask);
    printf("Tamanho do header %d\n",sizeof(ptr->bH));
    printf("-----------------------\n\n");
}


int hash( char *n)
{
	int i, h;

	for( i=0, h=1; n[i]!='\0'; ++i)
	{
		h = (h * n[i]) % TAMHASH;
	}

	printf("Funcao hash de <%s> deu <%d>\n", n, h);

	return h;
}


void insere( struct bmpHeader p, char *n)
{
	struct registro *novo;
	int h;

	if( (novo = (struct registro *) malloc( sizeof(struct registro))) == NULL )
	{	
      printf("Erro na alocacao\n");
			exit(1);
	}

	novo->data.bH = p;
	strcpy( novo->nome, n);

	h = hash(n);

	novo->prox = tabela[h];
	tabela[h] = novo;
}


struct registro * consulta( char *n)
{
	int h;
	struct registro *x;

	h = hash(n);

	x = tabela[h];

	while( x != NULL  &&  strcmp( n, x->nome) != 0 )
		x = x->prox;

	return x;
}

void *handle_requests()
{
  int PORT = 5010;
  int LEN = 255;
  char data_recv[LEN];
  int sockfd, ret;
  struct sockaddr_in servaddr, client;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("sockfd error\n");
  }
  else
  {
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    printf("binding at 0.0.0.0:%d\n", PORT);
    ret = bind(sockfd, (struct socketaddr *) &servaddr, sizeof(servaddr));
    if (ret < 0)
      perror("bind error\n");

    ret = listen(sockfd, 128);
    if (ret < 0)
      perror("listen error\n");

    printf("Listening in %s:%d", INADDR_ANY, PORT);
    int client_fd;
    socklen_t client_sz = sizeof(client);
    client_fd = accept(sockfd, (struct socketaddr *) &client, &client_sz);
    int n;
    while (1)
    {
      n = recv(client_fd, data_recv, LEN, 0);
      if (!n)
      {
        printf("Done receiving\n");
        break;
      }
      printf("received %d bytes: %s\n", n, data_recv);
    }
  }
}

void waitdata2insert()
{
    key_t key;
    int msgid;
		struct imgData to_recv;

    // ftok to generate unique key
    key = ftok("progfile", 65);

		printf("Create msg queue\n"); 
    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
 
		printf("Wait data on queue\n");

    while (1)
    {
      // msgrcv to receive message
      msgrcv(msgid, &to_recv, sizeof(to_recv), 1, 0);
       
      // display the message
      print_data(&to_recv);
      
      insere(to_recv.bH, to_recv.name);

      struct registro *p;
      p = consulta(to_recv.name);
      if (p == NULL)
        printf("Nao encontrou %s\n", to_recv.name);
      else
      {
        printf("Dado encontrado:\n");
        print_data(&(p->data));
      }
    }
    // to destroy the message queue
    msgctl(msgid, IPC_RMID, NULL);
}

int main( int argc, char *argv[])
{
	int i;
	for(i=0; i<TAMHASH; ++i)
		tabela[i] = NULL;
  
  pthread_t t;
  int ret;
  printf("creating thread\n");
  ret = pthread_create(&t, NULL, handle_requests, NULL);
  if (ret)
  {
    printf("pthread create failed\n");
    exit(1);
  }

	waitdata2insert();
/*
	for(i=0; i<TAMHASH; i++)
  {
    snprintf(name, sizeof(name), "0%d0", i);
    insere(i, name);
  }

  for(i=0; i<TAMHASH; i++)
  {
    snprintf(name, sizeof(name), "0%d0", i);
    p = consulta(name);

    if( p == NULL )
      printf("Nao encontrou %s!\n", name);
    else
      printf("Encontrou %s com prioridade %d!\n", name, p->prio);
  }
*/
}
	
