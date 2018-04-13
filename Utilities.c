//
// Created by pedro on 21-03-2018.
//
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

#define MAX_BUFFER_LENGHT 1024
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define CLOSE 0
#define OPEN 1

void parent_sigint_handler(int signo) {

    fprintf(stderr, "\nAre you sure you want to quit? (Y/N)");
    char option;
    scanf("%c", &option);

    if (option == 'Y' || option == 'y')
        exit(0);

    return;

}

char * createFileMessage(double time, int pid, char * filename){

    char * message;
    asprintf(&message, "%f - %d %s\n", time, pid, filename);

    return message;

}

void child_sigint_handler(int signo) {

    fprintf(stderr, "\nChild");

}

void findPatternInput(char *pattern) {

    char *buffer;

    char *pattern_location = NULL;

    size_t n;

    while (1) {

        getline(&buffer, &n, stdin);

        //removing the trailing newline
        buffer[strcspn(buffer, "\n")] = 0;


        if (strcmp(buffer, "q") == 0) {
            break;
        }

        if ((pattern_location = strstr(buffer, pattern)) != NULL) {

            printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET  "\n", buffer);

        }

    }

    free(buffer);

}

/*
 * search for the pattern, ignoring the case of both
 * to be used with -i flag
 *
 */

void findPatternFileInsensitive(char * filename, char * pattern){
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "could not open: %s\n", filename);
        exit(2);
    }

    char *buffer = NULL;

    size_t size = 0;

    char *word_location;

    size_t n = 0;

    int nLines = 0;

    while ((n = getline(&buffer, &size, file)) != -1) {

        ++nLines;

        if ((word_location = strcasestr(buffer, pattern)) != NULL) {

            printf("line %d:%s", nLines, buffer);

        }

    }

    free(buffer);
}

/*
 * search for the pattern, taking in consideration the case
 *
 */

void findPatternFile(char *filename, char *pattern) {

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "could not open: %s\n", filename);
        exit(2);
    }

    char *buffer = NULL;

    size_t size = 0;

    char *word_location;

    size_t n = 0;

    int nLines = 0;

    while ((n = getline(&buffer, &size, file)) != -1) {

        ++nLines;

        if ((word_location = strstr(buffer, pattern)) != NULL) {

            printf("line %d:%s", nLines, buffer);

        }

    }

    free(buffer);
    fclose(file);

}

/*
 * search for the full pattern in the file
 *
 */
void findWordFile(char *filename, char *pattern) {

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "could not open: %s\n", filename);
        exit(2);
    }

    char *buffer = NULL;

    size_t size = 0;

    char *word_location;

    size_t n = 0;

    int nLines = 0;

    while ((n = getline(&buffer, &size, file)) != -1) {

        ++nLines;

        if ((word_location = strstr(buffer, pattern)) != NULL) {

            /*
             * checking if word is not in the beginning of the sentence
             */
            if (word_location > buffer) {

                if(isalnum(*(word_location-1)))
                    continue;

                /*
                 * checking if the word is in the middle of the sentence
                 * so that we don't access invalid memory
                 */
                if(((word_location + strlen(pattern)) < ( buffer + strlen(buffer)))) {

                    if (!isalnum(*(word_location + strlen(pattern)))) {
                        printf("here\n");
                        printf("line %d:%s", nLines, buffer);
                    }

                }
                else{
                        printf("line %d:%s", nLines, buffer);
                }

            }else if(!isalnum(*(word_location + strlen(pattern)))){
                printf("line %d:%s", nLines, buffer);
            }



        }

    }

    free(buffer);


}

void updateLogFile(const char * env, char * message){
    char * file = getenv(env);
    FILE * fp = fopen(file,"a+");

    if ( fwrite(message, strlen(message), 1, fp) == 0){
        fprintf(stderr, "An error occurred while saving to the file\n");
        exit(1);
    }

    fclose(fp);
}


void analyse_directory(char *directory, char *pattern, const char * env) {

    DIR *dirp;
    struct dirent *direntp;
    struct stat stat_buf;
    char *name;

    int status;

    stat(directory, &stat_buf);

    /*
     * Need to test if it is a simple file,
     * if so, only do the normal search and exit
     */
    if (S_ISREG(stat_buf.st_mode)) {
        findPatternFile(directory, pattern);
        return;
    }


    if ((dirp = opendir(directory)) == NULL) {
        perror(directory);
        exit(1);
    }

    while ((direntp = readdir(dirp)) != NULL) {

        asprintf(&name, "%s/%s", directory, direntp->d_name);

        if (stat(name, &stat_buf) == -1) {
            perror("stat ERROR");
            exit(2);
        }

        if (S_ISREG(stat_buf.st_mode)) {
            updateLogFile(env, createFileMessage(0.15, getpid(), name));
            findPatternFile(name, pattern);
        }
            /*
             * Need to prevent it from opening the current and previous directory again
             */
        else if (S_ISDIR(stat_buf.st_mode) && (strcmp(direntp->d_name, ".") != 0) &&
                 ((strcmp(direntp->d_name, "..") != 0))) {


            pid_t pid = fork();
            if (pid == 0) {
                analyse_directory(name, pattern, env);
                return;
            }
            else {
                waitpid(pid, NULL, 0);
            }
        }

    }

    //free(name);


}

void setLogFile(const char * env){
    printf(">>What will be name of the logfile?\n");
    char * name;
    size_t size;


    int n = getline(&name, &size, stdin);

    //removing the trailing newline
    name[strcspn(name, "\n")] = 0;

    if (n == -1){
        fprintf(stderr, "An error occurred while reading the filename\n");
        exit(1);
    }
    else if(n == 0){
        fprintf(stderr, "An empty file name is not valid\n");
        setLogFile(env);
    }

    if (setenv(env, name, 1) != 0){
        fprintf(stderr, "An error occurred while adding the variable\n");
        exit(1);
    }

    printf(">>The variable %s with value %s was added with success\n", env, name);


}


int main(int argc, char *const argv[], char * envp[]) {

    /*
     * Setting up the env variable
     */
    const char * env = "LOGFILENAME";

    setLogFile(env);

    struct sigaction action;
    action.sa_handler = parent_sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) < 0) {
        fprintf(stderr, "Unable to install SIGINT handler\n");
        exit(1);
    }


    if (argc < 2) {
        printf("Usage: %s <pattern> <file>\n", argv[0]);
        exit(1);
    }

    char *pattern = argv[1];

    if (argc == 2) {
        findPatternInput(pattern);
        exit(0);
    }

    char *directory = argv[2];

    analyse_directory(directory, pattern, env);

    return 0;
}
