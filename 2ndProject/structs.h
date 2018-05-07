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


typedef struct{
    int time_out;
    int num_wanted_seats;
    int pref_seat_list[MAX_CLI_SEATS];
    char answer_fifo_name[MAX_ANS_FIFO];
}request_t;


typedef struct{
    int response_value;
    int num_reserved_seats;
    int * reserved_seat_list;
}answer_t;

#endif