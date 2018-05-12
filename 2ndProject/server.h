#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdbool.h>
#include "structs.h"

typedef struct{
    int clientID;
    bool reserved;
    pthread_mutex_t mutex;
} Seat;

typedef struct{
    pthread_t tid;
    request_t *request;
    answer_t *answer;
    Seat *seats;
    int ticket_office_num;
    int fd_slog;

    int fdAnswer;
} thread_t;

thread_t * new_thread(int ticket_office_num, int slog);
void free_thread(thread_t *thread);

void openAnswerFifo(thread_t *thread);
void readRequestThread(thread_t *thread);
void validateRequestThread(thread_t * thread);
int processRequest(thread_t * thread);
void unbookSeats(thread_t * thread);
void sendAnswer(thread_t *thread);
void sendFailedAnswer(thread_t * thread);
void validParameters(thread_t * thread);
void resetThread(thread_t *thread);
void displayAnswer(answer_t *answer);


typedef struct{
    Seat *room_seats;
    int num_room_seats;
    
    thread_t **ticket_offices;
    int num_ticket_offices;
    int open_time;           //In seconds

    int fdRequest;

    int fd_sbook;
    int fd_slog;

} server_t;


server_t * new_server();
void free_server(server_t *server);

void createRequestFifo(server_t *server);
void openRequestFifo(server_t *server);
void readRequestServer(server_t *server);

void createThreads(server_t *server);
void runThreads(server_t *server);
void endThreads(server_t *server);

void logOpenClose(int fd_slog, int ticket_office_num, bool toOpen);
void writeToBook(server_t * server);
void writeLog(thread_t * thread);
void logReservedSeats(thread_t * thread);


#endif