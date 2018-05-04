#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "structs.h"


request * createRequest(int xi, int yi){

    request * req = malloc(sizeof(request));

    req->x = xi;
    req->y = yi;
    req->pid = getpid();

    return req;

}

void sendRequest(request * req){

  int fd = open("fifo_req", O_WRONLY);

  if(fd == -1){
    fprintf(stderr, "Error opening fifo req\n");
    exit(1);
  }

  if ( write(fd, req, sizeof(request)) == -1){
    fprintf(stderr, "Error writing to fifo request\n");
    exit(2);
  }

}

answer * readAnswer(int fd_ans){

  answer * ans = (answer *) malloc(sizeof(answer));

  int n;

  while( (n = read(fd_ans,ans,sizeof(answer))) != -1){

    if(n == 0){
      continue;
    }else
      break;

  }

  close(fd_ans);

  return ans;

}


char * createAnswerFifo(int pid){
  char * name = NULL;

  if(asprintf(&name, "fifo_ans_%d", pid) == -1){
    fprintf(stderr, "unable to create name\n");
    exit(3);
  }

  if ( mkfifo(name,0755) == -1){
    fprintf(stderr, "Failed to create answer fifo\n");
    exit(4);
  }

  return name;
}

int main(){

  int _x;
  int _y;

  char * name = createAnswerFifo(getpid());

  while(1){
    printf("X and Y values: ");

    scanf("%d %d", &_x, &_y);

    request * req = createRequest(_x, _y);

    printf("Sending request to server...\n");

    sendRequest(req);

    printf("Request sent! Waiting for answer...\n");

    if(_x == 0 && _y == 0){
      remove(name);
      break;
    }

    int fd_ans = open(name, O_RDONLY);

    if(fd_ans == -1){
      fprintf(stderr, "Error opening answer fifo\n");
      exit(5);
    }

    answer * ans = readAnswer(fd_ans);

    printf("%d\n", ans->sum);

  }

  return 0;
}
