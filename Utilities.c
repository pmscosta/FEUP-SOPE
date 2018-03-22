//
// Created by pedro on 21-03-2018.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void findString(FILE *file, char *pattern) {

    char * buffer = NULL;

    size_t size = 0;

    char * word_location;

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


int main(int argc, char *const argv[]) {

    //Simple search word
    if(argc != 3){
        printf("Usage: %s <pattern> <file>\n", argv[0]);
        exit(1);
    }

    char * filename = argv[2];
    char * pattern = argv[1];


    FILE *file = fopen(filename, "r");

    if(file == NULL){
        fprintf(stderr, "%s: could not open: %s\n", argv[0], filename);
        exit(2);
    }


    findString(file, pattern);


    return 0;
}
