
#include <ctype.h>      // isalpha()
#include <fcntl.h>      // creat() 
#include <stdio.h>
#include <sys/wait.h>   // wait()
#include <string.h>     // strncpy()
#include <unistd.h>     // execvp()
#include <stdlib.h>     // exit()

char* PATH[16] = {"/bin", NULL};    // path of executable
int stdoutFD = 1;
int paraComIndex = 0;
extern int parallelCommand[16];     // in wish-v3.c

void isCommandBeforeRedirection(char*);
int parseLine(char*, char**);                   // in CommandConstructor.c
int isBuiltinCommand(char**, char**, int*);     // in CommandConstructor.c
char* executePath(char**, char**);              // in CommandConstructor.c
int DoRedirection(char*, int);                  // in Redirection.c 
int ParseForParallel(char*, char**);            // in ParseForParallel.c

int parallelCommand[16] = {0};  // 0 means no parallel command, otherwise 1

void eval(char*, int);

int
main(int argc, char** argv) {
    char cmdLine[128];  // command line

    if(argc == 1) {     // interactive mode
        while(1) {
            printf("wish> ");
            fgets(cmdLine, 128, stdin);
            if( feof(stdin) )  exit(0);

            eval(cmdLine, 0);
        }
    } 
    else {              // batch mode
        while(*++argv) {
            FILE* fpIn;
            if( (fpIn = fopen(*argv, "r")) == NULL ) {
                fprintf(stderr, "An error has occurred\n");
                exit(1);
            }

            while( fgets(cmdLine, 128, fpIn) ) {
                for(int i = 0; parallelCommand[i] != 0; i++) {
                    waitpid(parallelCommand[i], NULL, 0);
                }
                eval(cmdLine, 0);
            }
        }
    }

    return 0;
}


void eval(char* cmdLine, int isBackground) {
    char buf[128];      // holds modified command line
    strncpy(buf, cmdLine, 128);

    /** Does commands run in parallel? */
    if( strchr(buf, '&') != NULL) {
        char tmp[128];
        char* paraArgv[16];
        strncpy(tmp, buf, 128);
        isBackground = ParseForParallel(tmp, paraArgv);

        char* modifyBuf;  // modify buf for the command with no '&' behind
        modifyBuf = strrchr(buf, '&');  
        strncpy(buf, modifyBuf+1, 64);
    
        if(isBackground == 1) {  // run in parallel
            for(int i = 0; paraArgv[i] != NULL; i++)
                eval(paraArgv[i], 1);
        }
        isBackground = 0;  // parallel jobs done
    }

    /** Check whether fd of stdout is 1 */
    if(stdoutFD != 1 ) { 
        close(1);
        dup(stdoutFD);
    }

    /** Is there a '>' in buf? */
    for(int i = 0; i < strlen(buf); i++) {
        if( buf[i] == '>' ) {
            stdoutFD = DoRedirection(buf, i);
            break;
        }
    }

    /** Build the argv list for execvp() */
    char* argv[128]; 
    parseLine(buf, argv); 
    if(argv[0] == NULL) 
        return;         // Ignore empty lines

    pid_t pid;
    if( !isBuiltinCommand(argv, PATH, parallelCommand) ) {
        if( (pid = fork()) == 0 ) {  // child runs user job
            if( execv(executePath(PATH, argv), argv) < 0 ) {
                fprintf(stderr, "An error has occurred\n");
                exit(0);
            }
            
        } 

        /** Parent waits for foreground job to terminate */
        if( !isBackground ) { 
            int waitStatus;
            if( waitpid(pid, &waitStatus, 0) < 0 ) {
                fprintf(stderr, "child return code: %d\n", waitStatus);
                exit(1);
            }
        } else {  // parallel command add its pid to array
            parallelCommand[paraComIndex++] = pid;
        } 
    }
}

int ParseForParallel(char* buf, char** paraArgv) {
/** Parse the command line and build the argv array */
    buf[strlen(buf) - 1] = ' ';    // replace trailing '\n' with space
    while( *buf && (*buf == ' ') ) 
        buf++;  // ignore leading spaces

    /** Build the array list */
    int argc = 0;
    char* delim;
    while( (delim = strchr(buf, '&')) != NULL ) {
        paraArgv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while( *buf && (*buf == ' ') )
            buf++;  // ignore spaces
    }
    paraArgv[argc] = NULL;

    /** Are you a NULL string? */
    int isNullString = 1;  // 1 means null string, otherwise 0
    char tmpStr[32];
    strcpy(tmpStr, paraArgv[0]);
    for(int i = 0; i < strlen(tmpStr); i++) {
        if(tmpStr[i] >= 'a' || tmpStr[i] <= 'z') {
            isNullString = 0;
            break;
        }
    }
    if( isNullString == 1 )  exit(0);

    return 1;
}




int DoRedirection(char* buf, int redirPos) {
    /** Any commands before '>' ? */
    isCommandBeforeRedirection(buf);

    char* indicator = buf + redirPos + 1;
    while( *(indicator) == ' ' )  // ignore spaces
        indicator++; 

    /** Nothing behinds '>' */
    if( *indicator == '\n' ) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }

    /** Multiple files behinds '>', or number of '>' more than 1 */
    char* tmpPtr = strchr(indicator, ' ');
    if( tmpPtr != NULL ) {
        while( *tmpPtr == ' ' ) 
            tmpPtr++; 
        if( *tmpPtr != '\n' ) {
            if(*tmpPtr != 0x0) {
                fprintf(stderr, "An error has occurred\n");
                exit(0);
            }
        }
    }
    
    /** one file behinds '>' */
    char outputFile[32];
    sscanf(indicator, "%s", outputFile);

    int stdoutFD = dup(1);              // back up stdout
    int fd = creat(outputFile, 0644);   // create new file
    int newfd = dup2(fd, 1);            // redirects stdout to outputFile
    if( newfd == -1 ) {
        perror("dup2");  exit(1);
    }

    *(buf + redirPos) = '\0'; 
    return stdoutFD;
}


void isCommandBeforeRedirection(char* buf) {
    char* indicator = buf;
    int isCommandInside = 0;  // 0 means no command inside, otherwise 1
    while( *indicator != '>' ) {
        if( isalpha(*indicator) ) {
            isCommandInside = 1;
            break;
        }
        indicator++;
    }
    if( isCommandInside == 0 ) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }
}





int parseLine(char* buf, char** argv) {
/** Parse the command line and build the argv array */
    if( buf[strlen(buf) - 1] == '\n' )
        buf[strlen(buf) - 1] = ' ';         // replace trailing '\n' with space
    while( *buf && (*buf == ' ') ) {        // ignore leading spaces
        buf++;
    }
    
    /** Build the array list */
    int argc = 0; 
    char* delim;
    while( (delim = strchr(buf, ' ')) != NULL ) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while( *buf && (*buf == ' ') )   // ignore spaces
            buf++;
    }
    if( isalpha(*buf) ) 
        argv[argc++] = buf;
    argv[argc] = NULL;

    if(argc == 0)  // ignore blank line
        return 1;

    return 0;
}


int isBuiltinCommand(char** argv, char** PATH, int* paraCommand) {
/** If first arg is a builtin command, run it and return true */
    if( !strcmp(argv[0], "exit") ) {    // quit command
        if(argv[1] != NULL) 
            fprintf(stderr, "An error has occurred\n");
        for(int i = 0; paraCommand[i] != 0; i++) {
            waitpid(paraCommand[i], NULL, 0);
        }
        exit(0);
    }
    if( !strcmp(argv[0], "cd") ) {      // change directory
        if(argv[2] != NULL || argv[1] == NULL) { 
            fprintf(stderr, "An error has occurred\n");
            exit(0);
        }
        
        if( chdir(argv[1]) == -1 ) {
            perror("chdir");
            exit(1);
        }
        return 1;
    }
    if( !strcmp(argv[0], "path") ) {    // path command
        PATH[0] = NULL;
        int i = 1;
        while( argv[i] != NULL ) { 
            PATH[i-1] = strdup( argv[i] );
            i++;
        }
        PATH[i] = NULL;
        return 1;
    }
    return 0;
}


char* executePath(char** PATH, char** argv) {
/** Construct a executable path */
    char* exePath = (char*)malloc(64);
    memset(exePath, 0x0, 64);
    
    if( PATH[0] == NULL ) {
        strcpy(exePath, "no/path/setted/");
        return exePath;
    }

    for(int i = 0; PATH[i] != NULL; i++) {
        char tmpPath[64];
        strcpy(tmpPath, PATH[i]);
        if( tmpPath[ strlen(PATH[i])-1 ] != '/' ) {
            tmpPath[ strlen(PATH[i]) ] = '/';
            tmpPath[ strlen(PATH[i])+1 ] = '\0';
        }
        strcat(tmpPath, argv[0]);

        if( access(tmpPath, X_OK) == 0 ) 
            strncpy(exePath, tmpPath, 64);
    }

    return exePath;
}