#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdbool.h>
#include "structs.h"

/* 
Represents a Seat. 
Each seat will have it's own associated mutex.
 */
typedef struct{
    int clientID;  //the client ID (in this case, the PID of the process that reserved this seat)
    bool reserved; //indicates whether the seat is reserved or not
    pthread_mutex_t mutex; //mutex used to ensure mutual exclusion when tried to reserve the seat
} Seat;

/*
Unbooks the a certain seat,
@param seats - the array of seats
@param seatNum - the seat's number
*/
void freeSeat(Seat *seats, int seatNum);

/*
Books a certain seat for a certain client.
@param seats - the array of seats
@param seatNum - the seat's number
@param clientId - the client Identifier
*/
void bookSeat(Seat *seats, int seatNum, int clientId);

/*
See if a certain seat is free.
@param seat the array of seats
@param seatNum - the seat's number
@return 1 if it is free, 0 otherwise
*/
int isSeatFree(Seat *seats, int seatNum);

/*
Holds the information that each thread needs during it's run, for an easir access. 
*/
typedef struct{
    pthread_t tid;  //the threads tid
    request_t *request; //pointer to the current request that it is handling
    answer_t *answer; //pointer to the answer that will send after the request processing is done
    Seat *seats; //a redudant seat pointer in order to have acess to it out of the main part of the program
    int ticket_office_num; //the sequential creation number
    int fd_slog; //file descriptor of to the server log

    int fdAnswer; //file descriptor of the answer fifo to the client that made the request
} thread_t;

/*
Creates a new thread with all the necessary info stated above.
@param ticket_office_num - the sequential number, between 0 and the cinema's max number of ticket offices.
@param slog - a file descriptor pointing to the slog file of the cinema
*/
thread_t * new_thread(int ticket_office_num, int slog);

/*
Frees all the memory took up by the thread when the cinema running time ends
@param thread - the thread to be eliminated
*/
void free_thread(thread_t *thread);

/*
Tries to open the fifo in which the server will send the answer to the client.
@param thread - the thread that handled the request
@return 0 on sucess, -1 otherwise
*/
int openAnswerFifo(thread_t *thread);

/*
Reads the available request in the unitary buffer to the thread for processing.
Highly dependant on the program synchronization system.
@param thread - the thread that will process the request
*/
void readRequestThread(thread_t *thread);

/*
Validates the user request. Looks for errors mainly in seats identifiers and quantities.
Sets the answer value appropriately. 
@param thread - the thread containing the request to be handled. 
*/
void validateRequestThread(thread_t * thread);

/*
The main processing of the request. Tries to book the wanted seats for the user.
@param thread - the thread holding the request. 
@return 0 on sucess, 1 otherwise
*/
int processRequest(thread_t * thread);

/*
Frees the seats took up by a request that could not be totally met (i.e., couldn't reserve all the seats, only some)
@param thread- the thread holding the request
*/
void unbookSeats(thread_t * thread);

/*
Sends a valid answer to the user, after it was processed and validated.
@param thread - the thread containig the answer and the processed request
*/
void sendAnswer(thread_t *thread);

/*
Sends an unvalid answer to the user with the corresponding invalid answer value.
@param thread - the thread containig the answer and the processed request
*/
void sendFailedAnswer(thread_t * thread);

/*
Validates the user input on the request. Looks for repeated values, out of range values etc.
@param thread - the thread holding the request
@retur 0 on valid parameters, -1 otherwise
*/
int validParameters(thread_t * thread);

/*
Hard resets the thread after handling a request, clearing it's memory for another request. 
*/
void resetThread(thread_t *thread);

/*
Outputs the answer to be sent in stdout. Used only for debug matters.
*/
void displayAnswer(answer_t *answer);

/*
The main core of the server.
Holds all the information required by it during it's process, mainly it's thread and seats.
*/
typedef struct{
    Seat *room_seats;  //an array of all the seats in the cinema
    int num_room_seats; //the number of seats
    
    thread_t **ticket_offices; //all the threads
    int num_ticket_offices;  //the number of threas
    int open_time;           //In seconds

    int fdRequest; //file descriptor of the request fifo
    int fd_sbook; //file descriptor of the server's book log
    int fd_slog;  //file descriptor of the server's main log

} server_t;

/*
Creates a new server.
@return pointer to a new server.
*/
server_t * new_server();

/*
Frees the memory allocated by a server.
@param server - the server to be free'd
*/
void free_server(server_t *server);

/*
Creates the fifo through where the request will be received
@param server - the current server
*/
void createRequestFifo(server_t *server);

/*
Opens the created request fifo, waiting for request to be sent.
@param server - the current server
*/
void openRequestFifo(server_t *server);

/*
Continously reads from the request fifo and transfers them, when possible, to the unitary buffer
@param server - the current server 
*/
void readRequestServer(server_t *server);

/*
Creates all the desired threads at the beginning of the process
@param server - the current server
*/
void createThreads(server_t *server);

/*
Starts all the threads that will be waiting for available requests.
@param server - the server that created the threads
*/
void runThreads(server_t *server);

/*
Cancels all the threads at the end of the cinema's time.
@param server - the server that created the threads
*/
void cancelThreads(server_t *server);

/*
Waits for the threads to finisg
*/
void endThreads(server_t *server);

void logOpenClose(int fd_slog, int ticket_office_num, bool toOpen);
void writeToBook(server_t * server);
void writeLog(thread_t * thread);
void logReservedSeats(thread_t * thread);
void closeLogs(server_t *server);

#endif