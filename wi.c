#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define BUF_SIZE 50
const char err_msg[30] = "An error has occurred\n";
char here[255] = "./"; //globl variable used by builtin_path

int getsize(char array[]);
void trim(char **str);

void builtin_exit(char **args);
void builtin_cd(char **args);
void builtin_path(char **args, char ***path);

int main(int argc, char **argv) {

	int i = 0, ch_read;
	unsigned long n = 0;
	int pid;

	// input buffer
	char *buf = NULL;

	// path variable
	char **path = malloc(2*sizeof(char *));
	path[0] = "/bin/";
	path[1] = NULL;

	if (argc > 2) {
		write(STDERR_FILENO, err_msg, strlen(err_msg));
		exit(1);
	}

	FILE *fs;
	if (argc == 2) {
		if ((fs = fopen(argv[1], "r")) == NULL) {
			write(STDERR_FILENO, err_msg, strlen(err_msg));
			exit(1);
		}
	}

	while (1) {
		// interactive mode
		if (argc == 1) {
			printf("wish> ");
			ch_read = getline(&buf, &n, stdin);
		} else { // batch file mode
			ch_read = getline(&buf, &n, fs);
		}

		if (ch_read == -1) {
			if (argc != 1 && feof(fs)) {
				fclose(fs);
				exit(0);
			}
		}

		//replace newline at the end with nul character
		int bufsize = getsize(buf);
		buf[bufsize] = '\0';

		//first thing first, checks if it's a buildin
		char *tokens[BUF_SIZE] = {0};
		char *temp_buf = malloc(bufsize);
		memcpy(temp_buf, buf, bufsize);

		for (i=0; (tokens[i] = strsep(&temp_buf, " ")) != NULL; i++)
			;

		if (strcmp(tokens[0], "exit") == 0) { //exit builtin
			builtin_exit(tokens);
		} else if (strcmp(tokens[0], "cd") == 0) { //cd builtin
			builtin_cd(tokens);
			continue;
		} else if (strcmp(tokens[0], "path") == 0) { //path builtin
			builtin_path(tokens, &path);
			continue;
		}

		//parallel commands support with &
		char *programs[BUF_SIZE] = {0};

		for (i=0; (programs[i] = strsep(&buf, "&")) != NULL; i++)
			;

		int prog_length = i;

		//parallel command master loop
		int x;
		for (x=0; x < prog_length; x++) {



		//from buf to "command [> output_file]"
		char *command = programs[x];
		char *output_file = NULL;

		if (strstr(programs[x], ">") != NULL) {
			command = strsep(&programs[x], ">");
			trim(&programs[x]); //this is so boring tho
			output_file = strsep(&programs[x], " ");

			//if either no file or multiple files to redirect: error!
			if (strcmp(command, "") == 0 || strcmp(output_file, "") == 0 || output_file == NULL ||
					strsep(&programs[x], " ") != NULL) {
				write(STDERR_FILENO, err_msg, strlen(err_msg));
				break;
			}
		}

		//expand cmd into args "args[0] args[1] ..."
		char *args[BUF_SIZE] = {0};

		trim(&command);
		if (strcmp(command,"") == 0)
			continue;

		for (i=0; (args[i] = strsep(&command, " ")) != NULL; i++)
			;

		if ((pid = fork()) < 0) {
			write(STDERR_FILENO, err_msg, strlen(err_msg));
			exit(1);
		} else if (pid == 0) {

			// CHILD PROCESS CODE ////////////////////
			int path_size;
			char *cmd;

			//redirects output to output_file if needed
			if (output_file != NULL) {
				close(STDOUT_FILENO);
				close(STDERR_FILENO);
				open(output_file, O_WRONLY | O_CREAT, S_IRUSR);
			}

			//try path/binary existence
			int access_ret = -1;

			for(i=0; path[i] != NULL; i++) {
				path_size = getsize(path[i]);
				cmd = calloc (path_size + getsize(args[0]) + 1, sizeof(char));
				memcpy(cmd, path[i], path_size);
				strcat(cmd, args[0]);

				if ((access_ret = access(cmd, X_OK)) == 0) {
					execv(cmd, args);
					exit(1);
				}
			}

			if (access_ret == -1) {
				write(STDERR_FILENO, err_msg, strlen(err_msg));
				exit(1); //kills child
			}
		} // end of child process code
	} //end of master parallel loop

	int cpid;
	while ((cpid = wait(NULL)) > 0) ;
	if (argc == 1 && feof(stdin)) exit(0);
	if (argc > 1 && feof(fs)) exit(0);

	} //end of while(1) loop

	return 0;
}

int getsize(char *array) {
	int i = 0;

	while (array[i] != '\n' && array[i] != '\0')
		i++;

	return i;
}

void trim(char **str) {
	int i;
	int length = getsize(*str);
	char *trimmed = malloc(length * sizeof(char)); //this overrides path somehow

	int no_whitespace = 1;
	int y = 0;

	for (i=0; i < length; i++) {
		if (no_whitespace) {
			if (*(*str+i) != ' ') {
				trimmed[y++] = *(*str+i);
				no_whitespace = 0;
			}
		} else {
			trimmed[y++] = *(*str+i);

			if (*(*str+i) == ' ') {
				no_whitespace = 1;
			}
		}
	}

	if (no_whitespace) //remove possible trailing whitespace
		trimmed[y-1] = '\0';

	*str = trimmed;
}

//builtin functions
void builtin_exit(char **args) {
	if (args[1] != NULL) {
		write(STDERR_FILENO, err_msg, strlen(err_msg));
	}
	exit(0);
}

void builtin_cd(char **args) {
	if (args[1] != NULL && args[2] == NULL) {

		if (chdir(args[1]) == -1) //perform operation
			write(STDERR_FILENO, err_msg, strlen(err_msg));
	}
	else
		write(STDERR_FILENO, err_msg, strlen(err_msg));
}

void builtin_path(char **args, char ***path) {
	int i = 0;

	 do {
		 if (args[i+1] == NULL) {
				(*path)[i] = NULL;
		 } else {
			 (*path)[i] = malloc(sizeof(char *));

				if (args[i+1][0] == '/')
						//(*path)[i] = args[i+1];
						memcpy((*path)[i], args[i+1], strlen(args[i+1]));
				else {
					(*path)[i] = strcat(here, args[i+1]);
				}
				strcat((*path)[i], "/"); //add trailing slash
		 }
	} while ((*path)[i++] != NULL);
}