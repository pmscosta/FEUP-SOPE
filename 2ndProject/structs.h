#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#define DELAY(n) sleep(n)

#define CLIENT_LOG  "clog.txt"
#define SERVER_LOG  "slog.txt"
#define CLIENT_BKS  "cbook.txt"
#define SERVER_BKS  "sbook.txt"

#define WIDTH_PID   5
#define WIDTH_XXNN  5
#define WIDTH_SEAT  4

#define MAX_ROOM_SEATS  9999 
#define MAX_CLI_SEATS   99

#define MAX_ANS_FIFO    8

#define REQ_FIFO    "requests"
#define ANS_FIFO    "ans"

#define VALID_RESERVATION       0
#define INVALID_NUM_SEATS       -1
#define INVALID_PREF_NUMBER     -2
#define INVALID_SEATS_ID        -3
#define INVALID_PARAMETHERS     -4
#define UNAVALIABLE_SEAT        -5
#define ROOM_FULL               -6

typedef struct{
    int time_out;
    int num_wanted_seats;
    int num_pref_seats;
    int pid;
    int pref_seat_list[MAX_CLI_SEATS];
    char answer_fifo_name[MAX_ANS_FIFO];
}request_t;


typedef struct{
    int response_value;
    int num_reserved_seats;
    int * reserved_seat_list;
}answer_t;

#endif