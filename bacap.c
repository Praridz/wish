#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#define MAX_WORDS 100
/*const char *mypath[] = {
  "./",
  "/usr/bin/",
  "/bin/",
  NULL
};*/
char here[255] = "./"; 

typedef struct {
    char** tokens;
    int size;
} Tokens;

void cyan() {
  printf("\033[1;36m");
}

void yellow() {
  printf("\033[1;33m");
}

void reset() {
  printf("\033[0m");
}

char error_message[30] = "An error has occurred\n";
ssize_t getline(char **lineptr, size_t *n, FILE *stream);


int main(int argc, char *argv[]){
    FILE *fs;
    char **path = malloc(2*sizeof(char *));
	path[0] = "/bin/";
	path[1] = NULL;
    //Se toman en cuenta los parametros de entrada.
    if (argc > 2) {
        write(STDERR_FILENO, error_message, strlen(error_message)); 
		exit(1);
	}

	if (argc == 2) {
		if ((fs = fopen(argv[1], "r")) == NULL) {
			write(STDERR_FILENO, error_message, strlen(error_message)); 
			exit(1);
		}
	}
    int line = 0;
    bool corriendo = true;
    
    
    while (corriendo) {
        // empiezo declarando estos archivos asi , para que el getline haga el malloc automaticamente
        //declaro un apuntador que servira para guardar el comando o archivo batch
        char *linea = NULL;
        //solo puede ser positivo
        size_t bufsize = 0;
        //interactivo o batch
        if(argc == 1){
            cyan();
            printf("wish> ");
            reset();
            yellow();
            line = getline(&linea,&bufsize,stdin);
        }else{
            line = getline(&linea,&bufsize,fs);
        }
        //veo haber si no hay comandos en el archivo batch.
        if (line == -1) {
			if (argc != 1 && feof(fs)) {
				fclose(fs);
				exit(0);
			}
		}

        
        // tokenise the input line
        int buffer_size = 64;
        int position = 0;
        Tokens tokens;
        char* token;
        tokens.tokens = malloc(buffer_size * sizeof(char *));
        //se verifica si se hizo bien la asignacion de memoria con malloc
        if(!(tokens.tokens)){
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            exit(1);
        }
        //sepaara la linea , sin tener encuenta tab , newline , alert, return
        token = strtok(linea, " \t\r\n\a");
        while (token != NULL)
        {
            tokens.tokens[position] = token;
            position++;

            if(position >= buffer_size)
            {
                buffer_size += buffer_size;
                tokens.tokens = realloc(tokens.tokens, buffer_size * sizeof(char *));
                if(!tokens.tokens){
                    //se alcanzo el limite del malloc, y dio error en realloc , que servia para ampliar espacio
                    printf("realloc error\n");
                    exit(1);
                }
            }

            token = strtok(NULL, " \t\r\n\a");
        }
        //se asigna null a la ultima posicion del arreglo de separacion del comando.
        tokens.size = position;
        tokens.tokens[position] = NULL;

        char* command = tokens.tokens[0];
        if(strcmp(command, "cd") == 0){
            if(chdir(tokens.tokens[1]) != 0){
                //no se encontró el cd que se queria hacer
                write(STDERR_FILENO, error_message, strlen(error_message)); 
            }else{
                printf("Se ingresó a %s \n", tokens.tokens[1]);
            }
            
        } else if (strcmp(command, "exit") == 0){
            corriendo = false;
            printf("Cerrando consola \n");
            exit(0);
        }else if (strcmp(command, "path") == 0) { 
			printf("lol \n");
		}else{
                pid_t pid;
                pid = fork();








                if (pid == 0)
                {

                    int i;
                    for(i=0; path[i] != NULL; i++) {


                        if ((access_ret = access(cmd, X_OK)) == 0) {
					        execv(cmd, args);
					        exit(1);

                        }

				    }

                    /*
                    execv("/bin/ls", tokens.tokens);
                    printf("No entro\n");
                    */
                }
                else if (pid > 0)
                {  
                    wait(NULL);
                    printf("Finalizó\n");
                }

        }
    }


        return 0;
}