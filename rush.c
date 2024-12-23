//Karmichael Realina
//U73911380
//This program is a basic Unix shell that can take in and execute commands as well as handle redirection, parallel commands, and pathing
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/stat.h>

//global variables
char *line = NULL;
char *token;
char *path[255] = {"/bin", NULL};

//All functions
void error(); //outputs error msg
void getInput(); //gets user input
void parseInput(); //parses through input
void builtInCMDS(); //reads for built in commands like path, exit, and cd
void redirection(); //handles with redirection
void pathAccess(); //handles paths

//print error msg and exit
void error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    fflush(stdout);
}

//Gets User Input
void getInput() {
    size_t len = 0;
    ssize_t input;
    //gets user input
    input = getline(&line, &len, stdin);

    //if getline doesn't read input properly print error
    if(input == -1) {
        error();
        return;
    }

    //deletes "\n" in user input if exists
    line[strcspn(line, "\n")] = 0;
}

//Parses line or command
void parseInput(char* command, char* myargs[]) {
    char *arg = strdup(command);
    if(arg == NULL) {
        error();
        return;
    }

    int i = 0;
    while((token = strsep(&arg, " ")) != NULL) {
        if(*token != '\0') {
            myargs[i++] = token;
        }
    }
    myargs[i] = NULL;
    free(arg);
}

//checks for and handles built in commands like exit, cd, and path
void builtInCMDS(char* myargs[]) {
    if(myargs[0] == NULL) {
        return;
    }
    if(strcmp(myargs[0], "exit") == 0) {
        exit(0);
    } else if(strcmp(myargs[0], "cd") == 0) {
        if(myargs[1] == NULL || myargs[2] != NULL) {
            error();
        } else if(chdir(myargs[1]) != 0) {
            error();
        }
    } else if(strcmp(myargs[0], "path") == 0) {
        int j = 0;
        for(int i = 1; myargs[i] != NULL; i++) {
            path[j++] = myargs[i];
        }
        path[j] = NULL;
    }
}

//Handles redirection if the user prefers to send their output to a file rather than the terminal
void redirection(char* myargs[]) {
    int redirect = -1;
    int redirect_count = 0;
    for(int i = 0; myargs[i] != NULL; i++) {
        if(strcmp(myargs[i], ">") == 0) {
            redirect = i;
            redirect_count++;
        }
    }
    //if theres more than 1 redirection operators
    if(redirect_count > 1) {
        error();
        exit(1);
        return;
    }
    
    //redirects if as long as there is only 1 filename
    if(redirect != -1) {
        if(myargs[redirect + 1] == NULL || myargs[redirect + 2] != NULL) {
            error();
            exit(1);
            return;
        }
        close(STDOUT_FILENO);
        open(myargs[redirect+1], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
        myargs[redirect] = NULL;
    }
}

// Find the executable in the shell paths
void pathAccess(char* myargs[]) {
    char full_path[255];
    for(int i = 0; path[i] != NULL; i++) {
        snprintf(full_path, sizeof(path), "%s/%s", path[i], myargs[0]);
        if(access(full_path, X_OK) == 0) {
            execv(full_path, myargs);
            error(); // If execv fails, show error
            exit(1);
        }
    }
    error(); // If command not found, show error
}


int main(int argc, char *argv[]) {
    
    //checks if started with args
    if(argc > 1) {
        error();
        exit(1);
    }

    // Shell loop
    while (1) {

        //repeatedly prints prompt rush> and uses fflush to print everything in order
        printf("rush> ");
        fflush(stdout);

        getInput();
        char* command;
        char* rest = line;
        pid_t pids[255];
        int command_count = 0;

        // Loop through each command separated by '&'
        while ((command = strsep(&rest, "&")) != NULL) {
            if (strcmp(command, "") != 0) {
                char *myargs[255];
                parseInput(command, myargs);

                if (myargs[0] != NULL) {
                    // Handle built-in commands before forking
                    if (strcmp(myargs[0], "exit") == 0) {
                        if(myargs[1] != NULL) {
                            error();
                            continue;
                        } else {
                            exit(0);
                        }
                    } else if (strcmp(myargs[0], "cd") == 0 || strcmp(myargs[0], "path") == 0) {
                        builtInCMDS(myargs);
                        continue;
                    }

                    // Fork a process for external commands
                    pid_t pid = fork();
                    if (pid == -1) {
                        error();
                        exit(1);
                    } else if (pid == 0) {
                        // Child process
                        redirection(myargs); // Handle redirection
                        pathAccess(myargs);  // Execute command
                        exit(0);       // Ensure child exits after execution
                    } else {
                        // Parent process stores child PID
                        pids[command_count++] = pid;
                    }
                }
            }
        }

        // Wait for all child processes to complete
        for (int i = 0; i < command_count; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }

    free(line);
    return 0;
}