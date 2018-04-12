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

#define MAX_BUFFER_LENGHT 1024
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void parent_sigint_handler(int signo) {

    fprintf(stderr, "\nAre you sure you want to quit? (Y/N)");
    char option;
    scanf("%c", &option);

    if (option == 'Y' || option == 'y')
        exit(0);

    return;

}

void child_sigint_handler(int signo) {

    fprintf(stderr, "\nChild");

}

void findPatternInput(char *pattern) {

    char * buffer;

    char *pattern_location = NULL;

    size_t n;

    while (1){

        getline(&buffer, &n, stdin);

        //removing the trailing newline
        buffer[strcspn(buffer, "\n")] = 0;


        if(strcmp(buffer, "q") == 0){
            break;
        }

        if ((pattern_location = strstr(buffer, pattern)) != NULL) {

            printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET  "\n", buffer);

        }

    }

    free(buffer);

}

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

            printf("line %d: %s", nLines, buffer);

        }

    }

    free(buffer);

}

void analyse_directory(char *directory, char *pattern) {

    DIR *dirp;
    struct dirent *direntp;
    struct stat stat_buf;
    char *name;

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

            pid_t pid = fork();
            if (pid == 0) {
                findPatternFile(name, pattern);
                exit(0);
            } else {
                waitpid(pid, NULL, WNOHANG);
            }

        }
            /*
             * Need to prevent it from opening the current and previous directory again
             */
        else if (S_ISDIR(stat_buf.st_mode) && (strcmp(direntp->d_name, ".") != 0) &&
                 ((strcmp(direntp->d_name, "..") != 0))) {


            pid_t pid = fork();
            if (pid == 0) {
                analyse_directory(name, pattern);
                exit(0);
            } else {
                waitpid(pid, NULL, WNOHANG);
            }
        }

    }

    free(name);


}


int main(int argc, char *const argv[]) {

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

    struct sigaction action;
    action.sa_handler = parent_sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) < 0) {
        fprintf(stderr, "Unable to install SIGINT handler\n");
        exit(1);
    }

    analyse_directory(directory, pattern);

    return 0;
}
