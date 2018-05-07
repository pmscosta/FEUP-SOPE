#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdbool.h>
#include "structs.h"




typedef struct{
    pthread_t tid;
    request_t *request;
    answer_t *answer;

    int fdAnswer;
} thread_t;

thread_t * new_thread();
void free_thread(thread_t *thread);

void openAnswerFifo(thread_t *thread);
void readRequestThread(thread_t *thread);
void sendAnswer(thread_t *thread);




typedef struct{
    bool reserved;
} Seat;

typedef struct{
    Seat **room_seats;
    int num_room_seats;
    
    thread_t **ticket_offices;
    int num_ticket_offices;
    int open_time;           //In seconds

    int fdRequest;
    
    //Possivelmente deve ficar memoria partilhada


} server_t;

server_t * new_server();
void free_server(server_t *server);

void createRequestFifo(server_t *server);
void openRequestFifo(server_t *server);
void readRequestServer(server_t *server);

void createThreads(server_t *server);
void runThreads(server_t *server);
void endThreads(server_t *server);



#endif