#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "structs.h"

typedef struct{
    pid_t pid;

    request_t * request;
    answer_t * answer;

    int fdRequest;
    int fdAnswer;
    int fd_log;
    int fd_book;
    
    char * answer_fifo_name;
} client_t;

client_t * new_client();
void free_client(client_t *client);

void createAnswerFifo(client_t *client);
void openRequestFifo(client_t *client);
void openAnswerFifo(client_t *client);

void writeValidMessage(client_t *client, char *pid_msg );
void writeInvalidMessage(client_t *client, char *pid_msg);
void writeTimeOutMessage(client_t *client);

void createRequest(client_t *client, int time_out, int num_wanted_seats, int num_pref_seats, int *pref_seat_list);
void sendRequest(client_t *client);
void readAnswer(client_t *client);
void displayAnswer(answer_t *answer);

#endif