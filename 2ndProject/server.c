#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "server.h"

void * thr_run(void * thr){
  thread_t * thread = (thread_t *) thr;
  
  while(1){
    readRequestThread(thread);
    sendAnswer(thread);
  }
}


server_t * new_server(int num_room_seats, int num_ticket_offices, int open_time){
  
  server_t * server = (server_t *) malloc(sizeof(server_t));
  
  server->num_room_seats = num_room_seats;
  server->num_ticket_offices = num_ticket_offices;
  server->open_time = open_time;

  server->room_seats = (Seat**) malloc(server->num_room_seats * sizeof(Seat*));
  for(unsigned int i = 0; i < num_room_seats; i++){
    server->room_seats[i] = (Seat*) malloc(sizeof(Seat));
    (server->room_seats[i])->reserved = false;
  }

  server->request_buffer = (request_t *) malloc(sizeof(request_t));

  return server;
}

void free_server(server_t *server){
  for(unsigned int i = 0; i < server->num_room_seats; i++){
    free(server->room_seats[i]);
  }
  free(server->room_seats);
}

void createRequestFifo(server_t *server){
  if( mkfifo(REQ_FIFO, 0755) == -1){
    fprintf(stderr, "Unable to open fifo req\n");
    exit(1);
  }
}

void openRequestFifo(server_t *server){
  int fd = open(REQ_FIFO, O_RDONLY);

  if(fd == -1){
    fprintf(stderr, "error opening fifo req\n");
    exit(1);
  }
  server->fdRequest = fd;
}

void readRequestServer(server_t *server){
  int n;
  while( (n = read(server->fdRequest,server->request_buffer,sizeof(request_t))) != -1 ){

      if(n == 0){
        close(server->fdRequest);
        openRequestFifo(server);
        continue;
      }
  }
}

void createThreads(server_t *server){
  server->ticket_offices = (thread_t**) malloc(server->num_room_seats * sizeof(thread_t*));
  for(unsigned int i = 0; i < server->num_ticket_offices; i++){
    server->ticket_offices[i] = new_thread();
  }
}

void runThreads(server_t *server){
  for(unsigned int i = 0; i < server->num_ticket_offices; i++){
    pthread_create(&((server->ticket_offices[i])->tid), NULL, thr_run, &(server->ticket_offices[i]));
  }
}

void endThreads(server_t *server){

  for(unsigned int i = 0; i < server->num_ticket_offices; i++){
    pthread_join((server->ticket_offices[i])->tid, NULL);
    free_thread(server->ticket_offices[i]);
  }
  free(server->ticket_offices);
}




thread_t * new_thread(){
  thread_t * thread = (thread_t *) malloc(sizeof(thread_t));
  thread->request = (request_t *) malloc(sizeof(request_t)); 
  thread->answer = (answer_t *) malloc(sizeof(answer_t));
  return thread;
}

void free_thread(thread_t *thread){
  free(thread->answer);
  free(thread->request);
  free(thread);
}

void openAnswerFifo(thread_t *thread){
  int fd_ans = open(thread->request->answer_fifo_name, O_WRONLY);

  if(fd_ans == -1){
    fprintf(stderr, "Unable to open answer fifo\n");
    exit(3);
  }
  thread->fdAnswer = fd_ans;
}

void readRequestThread(thread_t *thread){

}

void sendAnswer(thread_t *thread){
  if ( write(thread->fdAnswer, thread->answer, sizeof(answer_t)) == -1){
    printf("error writing answer\n");
    exit(4);
  }
}




int isSeatFree(Seat *seats, int seatNum);
void bookSeat(Seat *seats, int seatNum, int clientId);
void freeSeat(Seat *seats, int seatNum);




int main(int argc,  char* argv[]){

  int num_room_seats = atoi(argv[1]);
  int num_ticket_offices = atoi(argv[2]);
  int open_time = atoi(argv[3]);

  server_t * server = new_server(num_room_seats, num_ticket_offices, open_time);
  createRequestFifo(server);
  openRequestFifo(server);

  createThreads(server);
  runThreads(server);

  //TODO: Usar variaveis de condicao para evitar busy waiting
  readRequestServer(server);

  endThreads(server);
  free_server(server);

  return 0;
}