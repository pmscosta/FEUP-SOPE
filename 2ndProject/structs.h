#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#define DELAY(n) sleep(n)

#define CLIENT_LOG  "clog.txt"
#define SERVER_LOG  "slog.txt"
#define CLIENT_BKS  "cbook.txt"
#define SERVER_BKS  "sbook.txt"

#define QUOTE(str) __QUOTE(str)
#define __QUOTE(str)  #str

#define WIDTH_PID   5
#define WIDTH_XXNN  5
#define WIDTH_XX    2
#define WIDTH_NN    2
#define WIDTH_SEAT  4

#define MAX_ROOM_SEATS  9999 
#define MAX_CLI_SEATS   99

#define MAX_ANS_FIFO    8

#define REQ_FIFO    "requests"
#define ANS_FIFO    "ans"
#define CLOG        "clog.txt"

#define VALID_RESERVATION        0

#define INVALID_NUM_SEATS       -1
#define MAX                     "MAX"

#define INVALID_PREF_NUMBER     -2
#define NST                     "NST"

#define INVALID_SEATS_ID        -3
#define IID                     "IID"

#define INVALID_PARAMETHERS     -4
#define ERR                     "ERR"

#define UNAVALIABLE_SEAT        -5
#define NAV                     "NAV"

#define ROOM_FULL               -6
#define FUL                     "FUL"

#define OUT                     "OUT"

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
    int reserved_seat_list[MAX_CLI_SEATS];
}answer_t;

#endif