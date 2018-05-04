#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "structs.h"
#include <fcntl.h>

void * calculate_sum(void * obj){
  request * obj2 = (request *) obj;
  int * result = malloc(sizeof(int));
  *result = obj2->x + obj2->y;
  return result;
}

void * calculate_diff(void * obj){
  request * obj2 = (request *) obj;
  int * result = malloc(sizeof(int));
  *result = obj2->x - obj2->y;
  return result;
}

void * calculate_mult(void * obj){
  request * obj2 = (request *) obj;
  int * result = malloc(sizeof(int));
  *result = obj2->x * obj2->y;
  return result;
}

void * calculate_div(void * obj){
  request * obj2 = (request *) obj;
  int * result = malloc(sizeof(int));
  *result = obj2->x / obj2->y;
  return result;
}


int main(){

  if( mkfifo("fifo_req", 0755) == -1){
    fprintf(stderr, "Unable to open fifo req\n");
    exit(1);
  }

  printf("Opening request fifo\n");
  int fd = open("fifo_req", O_RDONLY);

  if(fd == -1){
    fprintf(stderr, "error opening fifo req\n");
    exit(1);
  }

  request * req = (request *) malloc(sizeof(request));

  int n;
  while( (n = read(fd,req,sizeof(request))) != -1 ){

    if( n == 0){
      close(fd);
      fd = open("fifo_req", O_RDONLY);
      continue;
    }

    if(req->x == 0 && req->y == 0){
      close(fd);
      remove("fifo_req");
      break;
    }

    char * name = NULL;

    if(asprintf(&name, "fifo_ans_%d", req->pid) == -1){
      fprintf(stderr, "unable to create fifo name\n");
      exit(2);
    }

    //printf("name: %s\n", name);

    int fd_ans = open(name, O_WRONLY);

    if(fd_ans == -1){
      fprintf(stderr, "Unable to open answer fifo\n");
      exit(3);
    }

    answer * ans = malloc(sizeof(answer));

    /*
    ans->sum = *((int *) calculate_sum(req));
    ans->divd = *((int *) calculate_div(req));
    ans->mult = *((int *) calculate_mult(req));
    ans->diff = *((int *) calculate_diff(req));
    */
    ans->sum  = req->x + req->y;
    ans->divd = req->x / req->y;
    ans->mult = req->x * req->y;
    ans->diff = req->x - req->y;

    if ( write(fd_ans, ans, sizeof(answer)) == -1){
      printf("error writing answer\n");
      exit(4);
    }


  }
  return 0;
}
