
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

const char *mypath[] = {
  "./",
  "/usr/bin/",
  "/bin/",
  NULL
};
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

int main(int argc, char *argv[]){

    char *token;
    char *buffer;
    size_t bufsize = 32;
    size_t characters;

    buffer = (char *)malloc(bufsize * sizeof(char));
    if( buffer == NULL)
    {
        perror("Unable to allocate buffer");
        exit(1);
    }

    if (argc < 2)
    {
        printf("wgrep: searchterm [file ...]\n");
        //exit(1);
    }

    /** Dentro del main del intérprete*/
    while (1==1) {
    /* Wait for input */
    printf("wish> ");
    characters = getline(&buffer,&bufsize,stdin);
    printf("You typed: %s \n",buffer);


    int a =0;
    /* Parse input */
   while ((token = strsep(&buffer," ")) != NULL) {
       a++;
        printf("Numero: %d %s \n", a ,token);
        
        
        
   }
    
                


    /* If necessary locate executable using mypath array */
    /* Launch executable */
    //if (fork () == 0) {
     //   ...
      //  execv (...);
    //    ...
    //}
    //else
    //{
  //      wait (...);
   // }
    }


        return 0;
}