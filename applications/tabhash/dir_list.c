#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>


int main (void)
{
  DIR *dp;
  struct dirent *ep;

  dp = opendir ("./");
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
        strncpy(name_list[index], ep->d_name, sizeof(ep->d_name)+1);
        puts(*(name_list+index));
        index++;
        name_list = (char **) realloc(name_list, sizeof(name_list)*(index+1));

      }
    }
    printf("total: %d bmps", index);
    (void) closedir (dp);
  }
  else
    perror ("Couldn't open the directory");

  return 0;
}
