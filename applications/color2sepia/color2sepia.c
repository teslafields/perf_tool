#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* define inteiros de tamanho específico */
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <time.h>

#define sleep_ms 5000
#define iterations 10

int ITERATIONS = iterations;
unsigned long long tout = (unsigned long long) sleep_ms*1000*1000;

#define ubyte unsigned char

#pragma pack(push, 1) /* diz pro compilador nÃ£o alterar alinhamento 
                        ou tamanho da struct */
typedef  unsigned char RGBelement; // Cada eleemento RGB é representdao por 8 bits; um pixex tem 3 bytes (24 bits)


/*struct pixel{uint8_t r,g,b;};*/

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

#pragma pack(pop) /* restaura comportamento do compilador */


struct imgData {
  long mtype;
  char name[255];
	struct bmpHeader bH;
};


void printbin(char texto[], unsigned int valor)
{
     int i;
     printf("%s",texto);
     for (i=31;i>=0;i--) printf("%d",valor >> i & 1); 
     printf("\n");
}

void print_header(struct bmpHeader *bH)
{
    printf("Image size = %d x %d\n", bH->width, bH->height);
    printf("Tamanho do arquivo = %d\n", bH->size);
    printf("Number of colour planes is %hu\n", bH->planes);
    printf("Bits per pixel is %hu\n", bH->bits);
    printf("Compression type is %u\n", bH->compression);
    printf("Number of colours is %u\n", bH->ncolours);
    printf("Number of required colours is %u\n", bH->importantcolours);
    printf("X resolution: %d - Y resolution: %d (ps: pixels per meter)\n", bH->xresolution, bH->yresolution);
    printf("Header size is %d\n", bH->header_size);
    printbin("Red channel bitmask ", bH->redbitmask);
    printbin("Blue channel bitmask ", bH->bluebitmask);
    printbin("Green channel bitmask ", bH->greenbitmask);
    printbin("Alpha channel bitmask ", bH->alphabitmask);
    printf("Tamanho do bmp header %d\n",sizeof(struct bmpHeader));
    printf("-----------------------\n\n");

}

struct bmpHeader color2sepia(const char* source_dir, const char *pathname) {
    FILE *imagem;
    FILE *nova;
    char img_src[255];
    char img_tgt[255];
    int erro = 0;

    struct bmpHeader bH;
    RGBelement **matriz;

    RGBelement  valorReal;
    
    snprintf(img_src, sizeof(img_src) + sizeof(source_dir) + 1, "%s%s", source_dir, pathname);
    snprintf(img_tgt, sizeof(img_tgt) + 20, "bmp_out/%s_sepia.bmp", pathname);

//    printf("src: %s\ntgt: %s\n", img_src, img_tgt);
    imagem = fopen(img_src, "rb");
    nova = fopen(img_tgt, "wb");
    
    int sepiaDepth = 20;
  
    if (imagem == NULL)
        perror("Erro ao abrir a imagem");
    if (nova == NULL)
        perror("Erro ao abrir a nova");

    fread(&bH, sizeof(struct bmpHeader), 1, imagem);
    fwrite(&bH, sizeof(struct bmpHeader), 1, nova);
        
    matriz = (RGBelement **) malloc(sizeof(RGBelement *) * bH.height); 
    
    int i, j;
    
    for (i = 0; i < bH.height; i++) {
        matriz[i] = (RGBelement *) malloc(sizeof(RGBelement) * bH.width * 3); // Multplica por 3 porque cada pixel tem 3 bytes
    }
    
    fflush(stdout);
    /* leitura */
    for (i = 0; i < bH.height; i++) {
        for (j = 0; j < bH.width*3; j++) {
            fread(&matriz[i][j], sizeof(RGBelement), 1, imagem);
            // printf("%d ", matriz[i][j]);
            if (j>3 && j % 3 == 0){
            	int r = matriz[i][j];
            	int g = matriz[i][j - 1];
            	int b = matriz[i][j - 2];
	            int gry = (r + g + b) / 3;
            	r = g = b = gry;
            	r = r + (sepiaDepth * 2);
            	g = g + sepiaDepth;

            	if (r > 255) 
                	r = 255;
            	if (g > 255) 
                	g = 255;
	            if (b > 255) 
                	b = 255;
	            // Darken blue color to increase sepia effect
            	//b -= sepiaIntensity;

            	// normalize if out of bounds
            	if (b < 0) {
                	b = 0;
            	}
            	if (b > 255) {
                	b = 255;
            	}

            	matriz[i][j] = r;
            	matriz[i][j - 1] = g;
            	matriz[i][j - 2] = b;
        	}
   
		}
    }
    
    /* escrita */
    for (i = 0; i < bH.height; i++) {
        for (j = 0; j < bH.width*3; j++) {
            fwrite(&matriz[i][j], sizeof(RGBelement), 1, nova);
        }
        
    }
    
    unsigned char a;
    while (fread(&a,1,1,imagem))
       fwrite(&a,1,1,nova);
    
    for (i = 0; i < bH.height; i++) {
        free(matriz[i]);
    }
    free(matriz);
    
    //print_header(&bH);

    fclose(imagem);
    fclose(nova);
    return bH;
}


void send2hash(struct bmpHeader bH, char *name)
{
    key_t key;
    int msgid;
 		struct imgData to_send;

    to_send.mtype = 1;
    to_send.bH = bH;
    strcpy(to_send.name, name);
    // ftok to generate unique key
    key = ftok("progfile", 65);
    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
 
    // msgsnd to send message
    msgsnd(msgid, &to_send, sizeof(to_send), 0);
 
    // display the message
    printf("Data send is : %s \n", name);
 
}

void iterate_pictures(void)
{
  DIR *dp;
  struct dirent *ep;
  char source_dir[] = "bmp_in/";

  dp = opendir (source_dir);
  if (dp != NULL)
  {
    char *ptr;
    char **name_list;

    name_list = (char **) malloc(sizeof(char *));

    int index = 0;
    while (ep = readdir (dp))
    {
      ptr = strstr((const char *) ep->d_name, ".bmp");
      if (ptr != NULL)
      {
        name_list[index] = malloc(sizeof(char) * 255);
        strncpy(name_list[index], ep->d_name, sizeof(ep->d_name));
        index++;
        name_list = (char **) realloc(name_list, sizeof(name_list)*(index+1));

      }
    }
    (void) closedir (dp);

    struct bmpHeader msg;
    for (int k=0; k<index; k++)
    {
        msg = color2sepia(source_dir, name_list[k]);
		//		send2hash(msg, name_list[k]);
    }
  }
  else
    perror ("Couldn't open the directory");
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

int main()
{
  int ite = 0;
  struct timespec start;
  while ( ite < ITERATIONS )
  {
    clock_gettime(CLOCK_MONOTONIC, &start);
    iterate_pictures();
    nsleep(start);
    ite++;
  }
}
