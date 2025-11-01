/* Authors: Ahmet Oytun Kurtuldu 22120205009,Eren Tanyaş 22120205011,Ali İhsan Cengiz 22120205021 */

/* This program is a simple shell program that can execute shell commands and executable files. */
/* It also logs the commands to a file named log.txt. */
/* The program also has a built-in exit command to exit the shell. */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <glob.h>
void expand_wildcards(char *tokens[], int num_tokens);
void tokenize(char *user_input, char *tokens[], int num_tokens);
void print_tokens(char *tokens[], int num_tokens);
void add_paths(char *paths[], char *command);
void create_log_file();
void log_user_input(char *user_input);
void empty_tokens(char *tokens[], int num_tokens);
void check_exit(char *user_input);
void start_process(char *path, char *new_argv[]);
void find_command_path(char *command,char **path);
void get_user_input(char *user_input);
char *get_substring(char *str, int start, int end);
int main(){
    create_log_file(); /* creates the log file */
    while(true){ /* main loop */
        char *path = NULL; /* path of the command */
        char *new_argv[10]; /* arguments of the command */
        char *user_input = calloc(256,sizeof(char)); /* clears the user input */

        get_user_input(user_input); /* gets the user input */
        check_exit(user_input); /* checks if the user wants to exit */
        log_user_input(user_input); /* logs the command */
        tokenize(user_input, new_argv, 10); /* tokenizes the user input */
        find_command_path(new_argv[0],&path); /* finds the path of the command */
        start_process(path, new_argv); /* starts the process */

        free(user_input); /* frees the memory */
        for (int i = 0; i < 10; i++) {
            if (new_argv[i] != NULL) {
                free(new_argv[i]);
            }
        }

        if(path!=NULL){
            free(path); /* frees the memory */
        }
    }
    exit(0);
}
void create_log_file(){
    int opened_file = open("log.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644); /* creates the file */
    if(opened_file == -1){
        close(opened_file);
        perror("open");
        exit(1);
    }
    close(opened_file);
}
void get_user_input(char *user_input){
    write(1,"$",1); /* prints the '$' */

    int r = read(0,user_input,256); /* reads the user input */

    if(r <= 0){
        exit(1);
    }
        
    user_input[r-1] = '\0'; /* removes the newline character */
}
void check_exit(char *user_input){
    if(strncmp(user_input,"exit",4)==0){ /* checks if the user wants to exit */
            free(user_input); /* frees the memory */
            exit(0);
    }
}
void log_user_input(char *user_input){
    struct timeval tv;
    time_t t;
    struct tm *info;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec;
    info = localtime(&t);
    int opened_file = open("log.txt", O_WRONLY | O_APPEND, 0644); /* opens the file */
    if(opened_file == -1){
        perror("open");
        exit(1);
    }
    write(opened_file, asctime(info), strlen(asctime(info))-1); /* writes the time to the file */
    write(opened_file, "\t", 1);
    write(opened_file, user_input, strlen(user_input)); /* writes the command to the file */
    write(opened_file, "\n", 1);
    close(opened_file);
}
void tokenize(char *user_input, char *tokens[], int num_tokens){ /* tokenizes the user input */
    empty_tokens(tokens, num_tokens); /* empties the tokens */
    int count = 0;
    int i = 0;
    int start=0,end=0;

    while (user_input[i] != '\0') {  /* it iterates until it sees \0 */
        if(user_input[i]=='\"'){ /* if its a string start */
            i++;
            start = i;
            while(user_input[i]!='\"' && user_input[i] != '\0'){
                i++;
            }
            end = i;
            i++;
        }else if(user_input[i] == ' ' || user_input[i] == '\t' || user_input[i] == '\n'){ /* if its a space,tab or newline input */
            i++;
            continue;
        }else{
            start = i;
            while(user_input[i] != ' ' && user_input[i] != '\t' && user_input[i] != '\n' && user_input[i] != '\0'){
                i++;
            }
            end = i;
        }
        tokens[count] = get_substring(user_input,start,end); /* gets the substring */
        count++;
        i++;
    }
    expand_wildcards(tokens, num_tokens); /* expands the wildcards */
}
void empty_tokens(char *tokens[], int num_tokens){ /* empties the tokens */
    for(int i = 0; i < num_tokens; i++){
        tokens[i] = NULL;
    }
}
void start_process(char* path, char *new_argv[]) {
    if(new_argv[0] == NULL){
        return;
    }else{
        pid_t pid = fork(); /* creates a child process */
        int status; /* status of the child process */
        int i = 0;
        char **sh_args = NULL; /* arguments for the shell commands */

        if(pid == -1){
            perror("fork");
            exit(1);
        }
        else if(pid == 0) {
                int re = execv(path, new_argv); /* executes the command */
                
                if(re < 0) { /* checks if the command is executed */
                    if (access(path, X_OK) == 0) { /* checks if the command is an executable file in the given path */
                        execv(path, new_argv); /* execute the executable file */
                    }else if (access(path, F_OK) == 0) { /* check if the file exists in the given path */
                        /* this if statement is used for executing shell commands */
                        sh_args = calloc(13, sizeof(char*));  /* allocates memory for the arguments */

                        char *temp_path = calloc(128, sizeof(char)); /* temporary variable for the path */

                        find_command_path("bash",&temp_path); /* finds the path of the command */

                        sh_args[0] = "bash";  
                        sh_args[1] = path;  

                        for(i = 1; new_argv[i] != NULL; i++){
                            sh_args[i+1] = new_argv[i]; /* copies the arguments */
                        }

                        sh_args[i+1] = NULL; /* sets the last argument to NULL */
                        
                        int sh = execv(temp_path, sh_args); /* executes the shell command */

                        if(sh<0){ /* checks if the command is executed */
                            free(sh_args);
                            exit(1);
                        }
                    }else{
                        write(1,new_argv[0], strlen(new_argv[0]));
                        write(1,": command not found\n", 20);
                        exit(1);
                    }
                }

                exit(0); /* exits the child process */
        }
        else {
            wait(&status); /* waits for the child process */
            if(sh_args != NULL){
                free(sh_args);
            }

            /* checks the status of the child process */
            if (WIFEXITED(status)) {
                /*printf("Exited with status=%d\n", WEXITSTATUS(status));*/
            }else if(WIFSIGNALED(status)){
                printf("Killed by signal %d\n", WTERMSIG(status));
            }else if(WIFSTOPPED(status)){
                printf("Stopped by signal %d\n", WSTOPSIG(status));
            }else if(WIFCONTINUED(status)){
                printf("Continued\n");
            }
        }
    }
}
void find_command_path(char *command,char **path){
    if(command == NULL || command[0] == '\0'){
        return;
    }else{
        int new_out = open("output", O_WRONLY | O_TRUNC | O_CREAT, 0666); /* opens the file */
        char *temp = NULL; /* temporary variable for the path */

        if(new_out == -1){ /* checks if the file is opened */
            close(new_out);
            perror("open");
            exit(1);
        }

        int saved_out = dup(1); /* saves the output */
        int status; /* status of the child process */

        close(1); /* closes the output */

        dup2(new_out, 1); /* changes the output */

        pid_t pid = fork(); /* creates a child process */

        if(pid == -1){ /* checks if the fork is successfull */
            perror("fork");
            exit(1);
        }

        if(pid == 0){ /* child process */
            char *args[] = { "/usr/bin/which", command, NULL }; /* arguments for the command */
            int re = execv("/usr/bin/which",args); /* executes the command */

            if(re<0){ /* checks if the command is executed */
                perror("execv");
                exit(1);
            }

            exit(0);
        }
        if(pid > 0){ /* parent process */
            wait(&status);
        }
        
        dup2(saved_out, 1); /* restores the output */
        close(saved_out); /* closes the file */
        close(new_out); /* closes the file */

        int read_out = open("output", O_RDONLY); /* opens the file */

        char* buffer = (char*)calloc(256, sizeof(char)); /* buffer for the output */
        int r = read(read_out, buffer, 256); /* reads the output */

        if(r == -1){ /* checks if the file is read */
            perror("read");
            exit(1);
        }
        
        temp = strtok(buffer, "\n"); /* tokenizes the output */

        if(temp==NULL){ /* checks if the path is found */
            if (access(command, F_OK) == 0) {/* checks if the command is in the current directory */
                *path = strdup(command); /* copies the path to the path variable */
            }
        }

        if(temp!=NULL){ /* checks if the path is found */
            *path = (char*)calloc(strlen(temp)+1, sizeof(char)); /* allocates memory for the path */
            strcpy(*path, temp); /* copies the path to the path variable */
        }

        remove("output"); /* deletes the file */
        close(read_out); /* closes the file */
        free(buffer); /* frees the memory */
    }
}
char *get_substring(char *str, int start, int end){
    char *sub = (char*)calloc(end-start+1, sizeof(char)); /* allocates memory for the substring */
    int j = 0;

    for(int i = start; i < end; i++){ /* iterates until the end */
        sub[j] = str[i]; /* copies the character */
        j++;
    }

    return sub;
}
void expand_wildcards(char *tokens[], int num_tokens) {
    for (int i = 0; tokens[i] != NULL; i++) {
        /* this function handles wildcards */
        if (strchr(tokens[i], '*') != NULL || strchr(tokens[i], '?') != NULL) {
            glob_t glob_result;
            glob(tokens[i], GLOB_TILDE, NULL, &glob_result); /* expand the wildcard */

            for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                if (j == 0) {
                    strcpy(tokens[i], glob_result.gl_pathv[j]); /* replace token with expanded token */
                } else {
                    tokens[num_tokens++] = strdup(glob_result.gl_pathv[j]);
                }
            }

            globfree(&glob_result); /* free the memory */
        }
    }
}