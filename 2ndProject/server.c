#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "server.h"

pthread_mutex_t unit_buffer_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t server_cond = PTHREAD_COND_INITIALIZER;

int unit_buffer_full = 0;
int g_num_room_seats = 0;
request_t request_buffer;

void sigalarm_clean(server_t *data)
{
    static server_t *sServer;
    if (data) {
        sServer = data;
    } else {
        cancelThreads(sServer);
        closeLogs(sServer);
        free_server(sServer);
    }
}

void sigalarm_handler(int signo)
{
  fprintf(stderr, "Received alarm\n");
  sigalarm_clean(NULL);
  fprintf(stderr, "Exiting...\n");
  exit(1);
}

void sigalarm_install(){
  struct sigaction sa;

  sa.sa_handler = sigalarm_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGALRM, &sa, NULL) == -1)
  {
    fprintf(stderr, "Unable to install handler\n");
    exit(1);
  }
}


void displayAnswer(answer_t *answer)
{
  printf("--->Message\n");
  printf("response_value= %d\n", answer->response_value);
  printf("num_reserved_seats= %d\n", answer->num_reserved_seats);
}

void *thr_run(void *thr)
{
  thread_t *thread = (thread_t *)thr;

  while (1)
  {
    printf("reading request..\n");

    readRequestThread(thread);

    printf("read request\n");

    validateRequestThread(thread);

    if (thread->answer->response_value != 0)
    {

      printf("-------f------>invalid answer, return code = %d\n", thread->answer->response_value);
      sendFailedAnswer(thread);
      writeLog(thread);
      resetThread(thread);

      //TODO continue or break?
      continue;
    }
    printf("processing request\n");
    processRequest(thread);
    fprintf(stderr, "Sending answer ...\n");
    sendAnswer(thread);
    displayAnswer(thread->answer);
    writeLog(thread);
    resetThread(thread);
  }
}

void resetThread(thread_t *thread)
{
  thread->answer->response_value = 0;
  thread->answer->num_reserved_seats = 0;
  memset(thread->answer->reserved_seat_list, 0, sizeof(thread->answer->reserved_seat_list));
}

server_t *new_server(int num_room_seats, int num_ticket_offices, int open_time)
{

  server_t *server = (server_t *)malloc(sizeof(server_t));

  server->num_room_seats = num_room_seats;
  server->num_ticket_offices = num_ticket_offices;
  g_num_room_seats = num_room_seats;
  server->open_time = open_time;

  server->room_seats = (Seat *)malloc(server->num_room_seats * sizeof(Seat));
  for (unsigned int i = 0; i < num_room_seats; i++)
  {
    (server->room_seats[i]).reserved = false;
    (server->room_seats[i]).clientID = 0;

    if (pthread_mutex_init(&(server->room_seats[i].mutex), NULL) != 0)
    {
      fprintf(stderr, "Unable to init mutex\n");
      exit(5);
    }
  }

  return server;
}

void free_server(server_t *server)
{
  free(server->ticket_offices);
  free(server->room_seats);
  close(server->fdRequest);
  close(server->fd_sbook);
  close(server->fd_slog);
  
  if (unlink(REQ_FIFO) == -1)
  {
    perror("Error");
  }
}

void createRequestFifo(server_t *server)
{
  if (mkfifo(REQ_FIFO, 0755) == -1)
  {
    fprintf(stderr, "Unable to open fifo req\n");
    exit(1);
  }
}

void openRequestFifo(server_t *server)
{
  int fd = open(REQ_FIFO, O_RDONLY);

  if (fd == -1)
  {
    fprintf(stderr, "error opening fifo req\n");
    exit(1);
  }
  server->fdRequest = fd;
}

void displayRequest(request_t req)
{
  printf("\n\n-----------------_MESSAGE INFO------------------\n");
  printf("time_out %d \n", req.time_out);
  printf("num_wanted_seats %d \n", req.num_wanted_seats);
  printf("num_pref_seats %d \n", req.num_pref_seats);
  printf("fifo name %s \n", req.answer_fifo_name);
  printf("\n---------------------------------------------------\n");
}

void readRequestServer(server_t *server)
{
  int n;

  while (1)
  {
    printf("Locking server\n");
    pthread_mutex_lock(&unit_buffer_mut);
    fprintf(stderr, "Lock in Server\n");

    while (unit_buffer_full == 1)
    {
      pthread_cond_wait(&thread_cond, &unit_buffer_mut);
    }

    if ((n = read(server->fdRequest, &request_buffer, sizeof(request_t))) != -1)
    {
      if (n == 0)
      {
        close(server->fdRequest);
        openRequestFifo(server);
      }
      else
      {
        displayRequest(request_buffer);

        unit_buffer_full = 1;
        pthread_cond_signal(&server_cond);
        fprintf(stderr, "Send Condition Server\n");
      }
    }

    pthread_mutex_unlock(&unit_buffer_mut);
    printf("Unlock Server\n");
  }
}

void createThreads(server_t *server)
{
  server->ticket_offices = (thread_t **)malloc(server->num_room_seats * sizeof(thread_t *));

  for (unsigned int i = 0; i < server->num_ticket_offices; i++)
  {
    server->ticket_offices[i] = new_thread(i+1, server->fd_slog);
    logOpenClose(server->fd_slog, i + 1, true);
    server->ticket_offices[i]->seats = server->room_seats;
  }
}

void runThreads(server_t *server)
{
  for (unsigned int i = 0; i < server->num_ticket_offices; i++)
  {
    pthread_create(&((server->ticket_offices[i])->tid), NULL, thr_run, (server->ticket_offices[i]));
  }
}

void endThreads(server_t *server)
{
  for (unsigned int i = 0; i < server->num_ticket_offices; i++)
  {
    pthread_join((server->ticket_offices[i])->tid, NULL);
    free_thread(server->ticket_offices[i]);
    logOpenClose(server->fd_slog, i + 1, false);
  }
}

void cancelThreads(server_t *server)
{
  for (unsigned int i = 0; i < server->num_ticket_offices; i++)
  {
    pthread_cancel((server->ticket_offices[i])->tid);
    free_thread(server->ticket_offices[i]);
    logOpenClose(server->fd_slog, i + 1, false);
  }
}

thread_t *new_thread(int ticket_office_num, int slog)
{
  thread_t *thread = (thread_t *)malloc(sizeof(thread_t));
  thread->request = (request_t *)malloc(sizeof(request_t));
  thread->answer = (answer_t *)malloc(sizeof(answer_t));
  thread->ticket_office_num=ticket_office_num;
  thread->fd_slog = slog;
  return thread;
}

void free_thread(thread_t *thread)
{
  free(thread->answer);
  free(thread->request);
  free(thread);
}

void openAnswerFifo(thread_t *thread)
{
  int fd_ans = open(thread->request->answer_fifo_name, O_WRONLY);

  if (fd_ans == -1)
  {
    fprintf(stderr, "Unable to open answer fifo\n");
    exit(3);
  }
  thread->fdAnswer = fd_ans;
}

void readRequestThread(thread_t *thread)
{

  fprintf(stderr, "Locking Thread\n");
  pthread_mutex_lock(&unit_buffer_mut);
  printf("Locked in thread\n");

  while (unit_buffer_full == 0)
  {
    fprintf(stderr, "Wait Condition Server\n");
    pthread_cond_wait(&server_cond, &unit_buffer_mut);
  }
  printf("Critical\n");

  (*thread->request) = request_buffer;

  unit_buffer_full = 0;

  pthread_cond_signal(&thread_cond);
  pthread_mutex_unlock(&unit_buffer_mut);
  printf("Unlocked in thread\n");
}

//TO-DO any more cases?
void validParameters(thread_t *thread)
{
  //Invalid value
  if (thread->request->num_wanted_seats < 1 || thread->request->num_wanted_seats > thread->request->num_pref_seats)
    thread->answer->response_value = INVALID_PARAMETHERS;

  //Repeated seats
  else
  {
    int i = 0;

    bool repeated[g_num_room_seats + 1];
    memset(repeated, false, sizeof(repeated));

    for (; i < thread->request->num_wanted_seats; i++)
    {
      if (repeated[thread->request->pref_seat_list[i]])
      {
        thread->answer->response_value = INVALID_PARAMETHERS;
        return;
      }

      repeated[thread->request->pref_seat_list[i]] = true;
    }
  }
}

void validateRequestThread(thread_t *thread)
{

  //Seats quantity
  if (thread->request->num_wanted_seats > MAX_CLI_SEATS)
    thread->answer->response_value = INVALID_NUM_SEATS;

  //Valid preffered seats
  else if (thread->request->num_pref_seats + 1 < thread->request->num_wanted_seats || thread->request->num_pref_seats + 1 > MAX_CLI_SEATS)
  {
    printf("Wanted: %d\n Pref: %d\n", thread->request->num_wanted_seats, thread->request->num_pref_seats);
    thread->answer->response_value = INVALID_PREF_NUMBER;
  }

  //Seats identifiers
  else
  {
    int i = 0;

    for (; i < thread->request->num_wanted_seats; i++)
    {
      if (thread->request->pref_seat_list[i] < 1 || thread->request->pref_seat_list[i] > g_num_room_seats)
      {
        printf("Id:%d \n Max:%d\n", thread->request->pref_seat_list[i], g_num_room_seats);
        thread->answer->response_value = INVALID_SEATS_ID;
        return;
      }
    }
  }

  //Do we test this here (specs order), or in the beggining?
  validParameters(thread);
}

int isSeatFree(Seat *seats, int seatNum)
{
  return seats[seatNum].reserved != true;
}
void bookSeat(Seat *seats, int seatNum, int clientId)
{

  seats[seatNum].reserved = true;
  seats[seatNum].clientID = clientId;

  return;
}
void freeSeat(Seat *seats, int seatNum)
{
  printf("-------------------------------------------------------------->Freeing seat number %d\n", seatNum);
  seats[seatNum].reserved = false;
  seats[seatNum].clientID = 0;
}

bool roomFull(Seat *seats)
{
  for (int i = 1; i <= g_num_room_seats; i++)
  {
    if (seats[i].reserved == false)
      return false;
  }
  return true;
}

void unbookSeats(thread_t *thread)
{

  int i = 0;
  int id = thread->request->pid;

  //if the request failed, must free seats
  for (; i < thread->request->num_pref_seats; i++)
  {
    int seatNum = thread->request->pref_seat_list[i] - 1;

    pthread_mutex_lock(&(thread->seats[seatNum].mutex));

    if (thread->seats[seatNum].clientID == id)
      freeSeat(thread->seats, seatNum);

    pthread_mutex_unlock(&(thread->seats[seatNum].mutex));
  }
}

int processRequest(thread_t *thread)
{

  int id = thread->request->pid;

  if (roomFull(thread->seats))
  {
    thread->answer->response_value = ROOM_FULL;
    return 1;
  }

  int i = 0;

  int numberOfSeatsReserved = 0;

  for (; i < thread->request->num_pref_seats; i++)
  {

    int seatNum = thread->request->pref_seat_list[i] - 1;

    pthread_mutex_lock(&(thread->seats[seatNum].mutex));
    
    DELAY(1000);
    
    printf("Trying to reserve seat: %d\n", seatNum);

    if (isSeatFree(thread->seats, seatNum))
    {
      printf("seat is free %d\n", seatNum);
      bookSeat(thread->seats, seatNum, id);
      thread->answer->reserved_seat_list[numberOfSeatsReserved] = seatNum + 1;
      numberOfSeatsReserved++;
    }
    else
    {
      thread->answer->response_value = UNAVALIABLE_SEAT;
    }

    if (numberOfSeatsReserved == thread->request->num_wanted_seats)
    {
      thread->answer->response_value = VALID_RESERVATION;
      printf("-------------->reserved all seats\n");
      thread->answer->num_reserved_seats = numberOfSeatsReserved;
      numberOfSeatsReserved = 0;
      pthread_mutex_unlock(&(thread->seats[seatNum].mutex));
      return 0;
    }

    pthread_mutex_unlock(&(thread->seats[seatNum].mutex));
  }

  if (numberOfSeatsReserved != 0)
  {
    unbookSeats(thread);
  }

  numberOfSeatsReserved = 0;
  return 1;
}

void sendAnswer(thread_t *thread)
{

  printf("Opening answer fifo ...\n");
  openAnswerFifo(thread);

  if (write(thread->fdAnswer, thread->answer, sizeof(answer_t)) == -1)
  {
    printf("error writing answer\n");
    exit(4);
  }
}

void sendFailedAnswer(thread_t *thread)
{
  openAnswerFifo(thread);

  thread->answer->num_reserved_seats = 0;

  if (write(thread->fdAnswer, thread->answer, sizeof(answer_t)) == -1)
  {
    printf("error writing answer\n");
    exit(4);
  }
}

void badMessageAlloc()
{
  fprintf(stderr, "Unable to allocate memory for clog output\n");
  exit(7);
}

void logOpenClose(int fd_slog, int ticket_office_num, bool toOpen)
{

  char *openMessage = NULL;
  char *finalMessage = NULL;
  int nr = 0;

  char *msg_fmt = "%." QUOTE(WIDTH_TICKET_OFFICES) "d";

  nr = asprintf(&openMessage, msg_fmt, ticket_office_num);
  if (nr == -1)
    badMessageAlloc();

  if (toOpen)
    nr = asprintf(&finalMessage, "%s-OPEN", openMessage);
  else
    nr = asprintf(&finalMessage, "%s-CLOSED", openMessage);

  write(fd_slog, finalMessage, nr);
  write(fd_slog, "\n", 1);

  free(openMessage);
  free(finalMessage);
}

void openLogs(server_t *server)
{
  server->fd_sbook = open(SERVER_BKS, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
  
  if(server->fd_sbook == -1){
    fprintf(stderr, "Unable to create or open sbook file\n");
    exit(6);
  }

  server->fd_slog = open(SERVER_LOG, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
  if(server->fd_slog == -1){
    fprintf(stderr, "Unable to create or open fd_slog file\n");
    exit(6);
  }
}

void writeToBook(server_t *server)
{
  char * seat;
  
  char *seat_fmt = "%." QUOTE(WIDTH_SEAT) "d";

  for (int i = 0; i < g_num_room_seats; i++)
  {

    if (server->room_seats[i].reserved == true)
    {
      seat = NULL;
      int nr = asprintf(&seat, seat_fmt, i + 1);
      if (nr == -1)
        badMessageAlloc();

      write(server->fd_sbook, seat, nr);
      write(server->fd_sbook, "\n", 1);
    }
  }
  free(seat);
}

void closeLogs(server_t *server)
{
  write(server->fd_slog, "SERVER CLOSED", 13);
  writeToBook(server);
  close(server->fd_sbook);
  close(server->fd_slog);
}

void logReservedSeats(thread_t * thread){

  char * seat_fmt = "%." QUOTE(WIDTH_SEAT) "d";
  char * final_msg = NULL;
  char * seat = NULL;
  
  int i = 0;
  
  for (int j = 0; j < thread->answer->num_reserved_seats; j++)
  {
    i = asprintf(&seat, seat_fmt, thread->answer->reserved_seat_list[j]);
    if (i == -1)
      badMessageAlloc();

    i = asprintf(&final_msg, " %s",seat);
    if (i == -1)
      badMessageAlloc();

    write(thread->fd_slog,final_msg,i);
    free(seat);
    free(final_msg);
  }
}

void writeLog(thread_t * thread)
{

  //ID-Thread
  char * idThread=NULL;
  char * idThread_fmt = "%." QUOTE(WIDTH_TICKET_OFFICES) "d";
  int i = asprintf(&idThread, idThread_fmt, thread->ticket_office_num);

  if(i== -1)
    badMessageAlloc();
  
  //ID-Client
  char * idClient =NULL;
  char * idClient_fmt = "%." QUOTE(WIDTH_PID) "d";
  i = asprintf(&idClient, idClient_fmt, thread->request->pid);

  if(i== -1)
    badMessageAlloc();
  
  //Number of wanted seats 
  char * numSeats =NULL;
  char * numSeats_fmt = "%." QUOTE(WIDTH_SEAT) "d";
  i = asprintf(&numSeats, numSeats_fmt, thread->request->num_wanted_seats);

  if(i== -1)
    badMessageAlloc(); 

  char *final_msg = NULL;
  i = asprintf(&final_msg, "%s-%s-%s:",idThread , idClient, numSeats);
  if (i == -1)
    badMessageAlloc();

  write(thread->fd_slog, final_msg,i);
  free(idThread);
  free(idClient);
  free(numSeats);
  free(final_msg);


  char * seat=NULL;
  char * seat_fmt = "%." QUOTE(WIDTH_SEAT) "d";
  for (int j = 0; j < thread->request->num_pref_seats; j++)
  {
    if(thread->request->pref_seat_list[j] <1 ||thread->request->pref_seat_list[j]>g_num_room_seats) continue;

    i = asprintf(&seat, seat_fmt, thread->request->pref_seat_list[j]);
    if (i == -1)
      badMessageAlloc();

    i = asprintf(&final_msg, " %s",seat);
    if (i == -1)
      badMessageAlloc();

    write(thread->fd_slog,final_msg,i);
    free(seat);
    free(final_msg);
  }

  char *reason = NULL;

  switch (thread->answer->response_value)
  {
  case INVALID_NUM_SEATS:
    reason = MAX;
    break;
  case INVALID_PREF_NUMBER:
    reason = NST;
    break;
  case INVALID_SEATS_ID:
    reason = IID;
    break;
  case INVALID_PARAMETHERS:
    reason = ERR;
    break;
  case UNAVALIABLE_SEAT:
    reason = NAV;
    break;
  case ROOM_FULL:
    reason = FUL;
    break;
  default:
    break;
  }

  write(thread->fd_slog," -",2);
   if (reason != NULL)
  {
    i= asprintf(&final_msg," %s", reason);

    if (i == -1)
      badMessageAlloc();

    write(thread->fd_slog, final_msg, i);
    free(final_msg);
  } else logReservedSeats(thread);

  write(thread->fd_slog,"\n",1);
}

int main(int argc, char *argv[])
{

  if (argc != 4)
  {
    printf("Usage: %s <num_room_seats> <num_ticket_offices> <open_time>\n", argv[0]);
    exit(1);
  }

  int num_room_seats = atoi(argv[1]);
  int num_ticket_offices = atoi(argv[2]);
  int open_time = atoi(argv[3]);



  server_t *server = new_server(num_room_seats, num_ticket_offices, open_time);
 
  sigalarm_install();
  sigalarm_clean(server);

  createRequestFifo(server);
  openRequestFifo(server);
  openLogs(server);
  createThreads(server);
  
  alarm(open_time);
  
  runThreads(server);
  readRequestServer(server);

  endThreads(server);
  closeLogs(server);
  free_server(server);
  return 0;
}