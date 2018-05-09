#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "client.h"


client_t * new_client(){
  
  client_t * client = (client_t *) malloc(sizeof(client_t));
  
  client->request = (request_t *) malloc(sizeof(request_t));
  client->answer = (answer_t *) malloc(sizeof(answer_t));
  
  client->pid = getpid();

  return client;
}

void createRequest(client_t *client, int time_out, int num_wanted_seats, int num_pref_seats, int *pref_seat_list){
    
    request_t * req = client->request;

    req->time_out = time_out;
    req->num_wanted_seats = num_wanted_seats;
    req->num_pref_seats = num_pref_seats;
    memcpy(req->pref_seat_list, pref_seat_list, MAX_CLI_SEATS);
    memcpy(req->answer_fifo_name, client->answer_fifo_name, MAX_ANS_FIFO);
}

void openRequestFifo(client_t *client){
  int fd = open(REQ_FIFO, O_WRONLY);

  if(fd == -1){
    perror("Error");
    fprintf(stderr, "Failed to open requests fifo\n");
    exit(1);
  }
  client->fdRequest = fd;
}

void createAnswerFifo(client_t *client){
  char * name = NULL;

  if(asprintf(&name, ANS_FIFO"%d", client->pid) == -1){
    fprintf(stderr, "unable to create name\n");
    exit(3);
  }

  if (mkfifo(name,0755) == -1){
    fprintf(stderr, "Failed to create answer fifo\n");
    exit(4);
  }

  client->answer_fifo_name = name;
}

void openAnswerFifo(client_t *client){
  int fd_ans = open(client->answer_fifo_name, O_RDONLY);

  if(fd_ans == -1){
    fprintf(stderr, "Error opening answer fifo\n");
    exit(5);
  }

  client->fdAnswer = fd_ans;
}

void sendRequest(client_t *client){
  //TODO: Possiveis erros: verificar valor do fd e do req

  if (write(client->fdRequest, client->request, sizeof(request_t)) == -1){
    perror("Error");
    fprintf(stderr, "Error writing to fifo request\n");
    exit(2);
  }

}

void readAnswer(client_t *client){

  int n;

  while( (n = read(client->fdAnswer,client->answer,sizeof(answer_t))) != -1){

    if(n == 0){
      continue;
    }else
      break;

  } 

}

void displayAnswer(answer_t *answer){
  printf("--->Message\n");
  printf("response_value= %d\n", answer->response_value);
  printf("num_reserved_seats= %d\n", answer->num_reserved_seats);
}

void free_client(client_t *client){
  close(client->fdRequest);
  free(client->request);

  close(client->fdAnswer);
  if(unlink(client->answer_fifo_name) == -1){
    perror("Error");
  }
  free(client->answer);
  free(client->answer_fifo_name);
}


int main(int argc, char *argv[]){

  //TODO: Testar possiveis erros;
  
  int time_out = atoi(argv[1]);
  int num_wanted_seats = atoi(argv[2]);
  int pref_seat_list[num_wanted_seats];

  unsigned int index = 0; 

  //Possivel erro Out of range se introduzidos mais valores que os especificados
  for(unsigned int i = 3; argv[i] != NULL; i++, index++){
    pref_seat_list[index] = atoi(argv[i]);
  }

  client_t * client = new_client();
  
  createAnswerFifo(client);
  createRequest(client, time_out, num_wanted_seats, index, pref_seat_list);
  openRequestFifo(client);
  printf("Sending request ...\n");
  sendRequest(client); 
  printf("Close request ...\n");
  close(client->fdRequest);
  printf("Opening answer fifo ...\n");
  openAnswerFifo(client);
  printf("Reading answer from fifo ...\n");
  readAnswer(client);
  displayAnswer(client->answer);
  printf("Exit ...\n");
  free_client(client);

  return 0;
}