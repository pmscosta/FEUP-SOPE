#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "structs.h"
/*
Holds all the information relative to a client.
*/
typedef struct{
    pid_t pid; //the calling process pid

    request_t * request; //pointer to the request made by this client
    answer_t * answer; //pointer to the answer received by this client

    int fdRequest; //file descriptor of the fifo where the request is sent
    int fdAnswer; //file descriptor of the fifo through which the answer will be sent
    int fd_log; //file descriptor of the file where the main log is kept
    int fd_book; //file descriptor of the file where the log relative to the seats number is kept
    
    char * answer_fifo_name; //the name given to the answer fifo 
} client_t;


/*
Creates a new client
@return pointer to the new client
*/
client_t * new_client();

/*
Deletes/Frees a client
@param client - the client to be deleted
*/
void free_client(client_t *client);

/*
Creates the fifo through where the answer will be received
@param cluent - the current client
*/
void createAnswerFifo(client_t *client);
/*
Opens the request fifo, where the request will be sent.
@param client - the current client
*/
void openRequestFifo(client_t *client);

/*
Opens the created answer fifo
@param client - the current client
*/
void openAnswerFifo(client_t *client);

/*
Writes the client log the the correspondent file.
@param client
*/
void writeLog(client_t *client);
/*
When the request was approved an thus the answer valid, writes a message
portraying that in the log file
@param client
@param pid_msg - a string already containing the pid of the calling process
*/
void writeValidMessage(client_t *client, char *pid_msg);
/*
When the request was not approved, writes a message
portraying that in the log file
@param client
@param pid_msg - a string already containing the pid of the calling process
*/
void writeInvalidMessage(client_t *client, char *pid_msg);

/*
When the answer does not arrive before the specified timeout, 
writes a special message to the logfile
@param clinet 
*/
void writeTimeOutMessage(client_t *client);

/*
Converts a string to integer array. Useful for parsing information from the command line arguments.
@param pref_list - the string to be converted
@param pref_seat_list - the array being created
@return the number of elements in the array
*/
int string_to_array(char *pref_list, int **pref_seat_list);

/*
Creats a new request with the information passed by the arguments
@param client - the client that made the request
@param time_out - available waiting time
@param num_wanted_seats - the number of seats wanted
@param num_pref_seats - the number of preferres seats
@param pref_seat_list - an array containing all the preffered seats
*/
void createRequest(client_t *client, int num_wanted_seats, int num_pref_seats, int *pref_seat_list);

/*
Sends the request through the request fifo.
@param client - the client that made the request
*/
void sendRequest(client_t *client);

/*
Waits for the answer to be sent by the server and reads it.
@param client - the client waiting for the answer
*/
void readAnswer(client_t *client);

/*
Displays the given answer in stdout. Only used for debug purposes.
@param client - the clien that made the request
*/
void displayAnswer(answer_t *answer);

#endif