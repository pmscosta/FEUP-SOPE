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
#include <sys/time.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_BUFFER_LENGHT 1024
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define CLOSE 0
#define OPEN 1

typedef struct {
    bool ignoreCase;
    bool showOnlyFileName;
    bool showLineNumber;
    bool showNumberOfLines;
    bool completeWord;
    bool recursive;
} options;


/*
 * store program initial time
 */
struct timeval t_init;
struct timeval t_end;


double getTime() {
    gettimeofday(&t_end, NULL);
    double time = (double) (t_end.tv_usec - t_init.tv_usec) / 1000000 + (double) (t_end.tv_sec - t_init.tv_sec);
    time *= 1000;
    return time;
}

void parent_sigint_handler(int signo) {
    int repeat = 1;

    while (repeat) {
        repeat = 0;
        fprintf(stderr, "\nAre you sure you want to quit? (Y/N): ");
        char option;
        scanf("%c", &option);

        if (option == 'Y' || option == 'y') {
            kill(-getpgrp(), SIGUSR1);
            exit(0);
        } else if (option == 'N' || option == 'n') {
            kill(-getpgrp(), SIGUSR2);
        } else {
            repeat = 1;
        }
    }
}


void child_sigint_handler(int signo) {
    switch (signo) {
        case SIGINT:
            pause();
            break;
        case SIGUSR1:
            exit(0);
            break;
        case SIGUSR2:
            break;
    }

}

void parent_sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


void updateLogFile(const char *env, char *message) {
    char *file = getenv(env);
    FILE *fp = fopen(file, "a+");

    if (fwrite(message, strlen(message), 1, fp) == 0) {
        fprintf(stderr, "An error occurred while saving to the file\n");
        exit(1);
    }

    fclose(fp);
}


char *createFileMessage(double time, int pid, char *filename, int mode) {

    char *message = NULL;

    char *action = NULL;

    int n;

    if (mode == OPEN) {
        n = asprintf(&action, "Opened %s", filename);
    } else {
        n = asprintf(&action, "Closed [%s]", filename);
    }

    if (n == -1 || asprintf(&message, "%.2f - %d %s\n", time, pid, action) == -1) {
        fprintf(stderr, "Unable to create log message to file\n");
        exit(1);
    }

    return message;

}


bool completeWord(char *buffer, char *pattern, char *word_location) {


    if (word_location > buffer) {

        if (isalnum(*(word_location - 1)))
            return false;

        /*
         * checking if the word is in the middle of the sentence
         * so that we don't access invalid memory
         */
        if (((word_location + strlen(pattern)) < (buffer + strlen(buffer)))) {

            if (!isalnum(*(word_location + strlen(pattern)))) {
                return true;
            }

        } else {
            return true;
        }

    } else if (!isalnum(*(word_location + strlen(pattern)))) {
        return true;
    }

    return false;
}

void findPatternInput(char *pattern, options *op) {

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
            if (((pattern_location = strstr(buffer, pattern)) != NULL && !op->ignoreCase) ||
                (op->ignoreCase && (pattern_location = strcasestr(buffer, pattern)) != NULL)) {

                if (!op->completeWord || completeWord(buffer, pattern, pattern_location)) {
                    printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET  "\n", buffer);
                }
            }

        }

    }

    free(buffer);

}

/*
 * search for the pattern, taking in consideration the case
 *
 */

void findPatternFile(char *filename, char *pattern, options *op, const char *env) {

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

    int lineCounter = 0;


    while ((n = getline(&buffer, &size, file)) != -1) {
        ++nLines;

        if (((word_location = strstr(buffer, pattern)) != NULL && !op->ignoreCase) ||
            (op->ignoreCase && (word_location = strcasestr(buffer, pattern)) != NULL)) {

            /*
             * if the word was found and we only need to print the filename
             */
            if (op->showOnlyFileName) printf("%s\n", filename);
            else {

                if (!op->completeWord || completeWord(buffer, pattern, word_location)) {
                    if (op->showNumberOfLines) lineCounter++;
                    else {
                        if (op->showLineNumber) printf("%d:", nLines);
                        printf("%s", buffer);
                    }
                }
            }
        }
    }
    if (op->showNumberOfLines) printf("%d\n", lineCounter);

    free(buffer);

    updateLogFile(env, createFileMessage(getTime(), getpid(), filename, CLOSE));
    fclose(file);

}


void analyse_directory(char *directory, char *pattern, options *op, const char *env) {

    DIR *dirp;
    struct dirent *direntp;
    struct stat stat_buf;
    char *name;


    stat(directory, &stat_buf);

    /*
     * Need to test if it is a simple file,
     * if so, only do the normal search and exit
     */
    if (S_ISREG(stat_buf.st_mode)) {
        findPatternFile(directory, pattern, op, env);
        return;
    } else if (S_ISDIR(stat_buf.st_mode)) {
        if (!op->recursive) {
            printf("%s is a directory\n", directory);
            exit(0);
        }
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
            updateLogFile(env, createFileMessage(getTime(), getpid(), name, OPEN));
            findPatternFile(name, pattern, op, env);
        }
            /*
             * Need to prevent it from opening the current and previous directory again
             */
        else if (S_ISDIR(stat_buf.st_mode) && (strcmp(direntp->d_name, ".") != 0) &&
                 ((strcmp(direntp->d_name, "..") != 0)) && op->recursive) {

            sigset_t signal_set;
            sigemptyset(&signal_set);
            sigaddset(&signal_set, SIGUSR1);
            sigaddset(&signal_set, SIGUSR2);
            sigaddset(&signal_set, SIGINT);
            sigprocmask(SIG_BLOCK, &signal_set, NULL);

            pid_t pid = fork();
            if (pid == 0) {
                struct sigaction action;
                action.sa_handler = child_sigint_handler;
                sigemptyset(&action.sa_mask);
                action.sa_flags = 0;

                if (sigaction(SIGINT, &action, NULL) < 0) {
                    fprintf(stderr, "Unable to install SIGINT handler\n");
                    exit(1);
                }

                if (sigaction(SIGUSR1, &action, NULL) < 0) {
                    fprintf(stderr, "Unable to install SIGUSR1 handler\n");
                    exit(1);
                }

                if (sigaction(SIGUSR2, &action, NULL) < 0) {
                    fprintf(stderr, "Unable to install SIGUSR2 handler\n");
                    exit(1);
                }

                sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
                analyse_directory(name, pattern, op, env);
                return;
            } else {
                sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
                //waitpid(pid, NULL, 0);
            }

        }

    }

    while (wait(NULL) > 0);
    free(name);
}

void setLogFile(const char *env) {
    printf(">>What will be name of the logfile?\n");
    char *name = NULL;
    size_t size;

    int n = getline(&name, &size, stdin);

    if (n == -1) {
        fprintf(stderr, "An error occurred while reading the filename\n");
        exit(1);
    } else if (n == 0) {
        fprintf(stderr, "An empty file name is not valid\n");
        setLogFile(env);
    }

    //removing the trailing newline
    name[strcspn(name, "\n")] = 0;


    if (setenv(env, name, 1) != 0) {
        fprintf(stderr, "An error occurred while adding the variable\n");
        exit(1);
    }

    printf(">>The variable %s with value %s was added with success\n", env, name);
}

void reset(options *op) {
    op->ignoreCase = false;
    op->showOnlyFileName = false;
    op->showLineNumber = false;
    op->showNumberOfLines = false;
    op->completeWord = false;
    op->recursive = false;
}


void analyseAction(int argc, char *const argv[], const char *env) {


    char *pattern;

    options *op = (options *) malloc(sizeof(options));
    reset(op);

    if (argc == 2) {
        pattern = argv[1];
        findPatternInput(pattern, op);
        exit(0);
    }

    int i;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-i") == 0) { op->ignoreCase = true; }
            else if (strcmp(argv[i], "-l") == 0)op->showOnlyFileName = true;
            else if (strcmp(argv[i], "-n") == 0)op->showLineNumber = true;
            else if (strcmp(argv[i], "-c") == 0)op->showNumberOfLines = true;
            else if (strcmp(argv[i], "-w") == 0) op->completeWord = true;
            else if (strcmp(argv[i], "-r") == 0) op->recursive = true;
        } else break;
    }


    char *directory;

    if (i == argc - 2) {
        directory = argv[argc - 1];
        pattern = argv[i];
        analyse_directory(directory, pattern, op, env);
    } else if (i == argc - 1) {
        pattern = argv[i];
        if (op->showLineNumber || op->showOnlyFileName || op->recursive || op->showNumberOfLines)
            printf("Invalid flags for a STDIN read\n");
        else
            findPatternInput(pattern, op);
    } else printf("Unable to recognize flags\n");

}

int main(int argc, char *const argv[]) {

    if (argc < 2) {
        printf("Usage: %s <pattern> <file>\n", argv[0]);
        exit(1);
    }


    gettimeofday(&t_init, NULL);

    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    action.sa_handler = parent_sigint_handler;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        fprintf(stderr, "Unable to install SIGINT handler\n");
        exit(1);
    }

    action.sa_handler = SIG_IGN;
    if (sigaction(SIGUSR1, &action, NULL) < 0) {
        fprintf(stderr, "Unable to install SIGUSR1 handler\n");
        exit(1);
    }
    if (sigaction(SIGUSR2, &action, NULL) < 0) {
        fprintf(stderr, "Unable to install SIGUSR2 handler\n");
        exit(1);
    }

    action.sa_handler = parent_sigchld_handler;
    if (sigaction(SIGCHLD, &action, NULL) < 0) {
        fprintf(stderr, "Unable to install SIGCHLD handler\n");
        exit(1);
    }

    /*
     * Setting up the env variable
     */
    const char *env = "LOGFILENAME";

    setLogFile(env);

    fprintf(stderr, "Analyse\n");
    analyseAction(argc, argv, env);


    return 0;
}