#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>

#include "gmalloc.h"

#define MAX_BUFFER 255
#define MAX_CONN 1024
#define MAX_MSG_LENGTH 99999

char* root_path;
char* port;
int listenfd, clients[MAX_CONN];

void initialize_environments(void** root_path, void** port);
void cleanup_environments();
void start_server();
void respond(int);

// Setup Unix signal handlers for this simple HTTP server.
void setup_signals();
void connect_signal(int signum, void (*func) (int));
void end_me(int signum);

void print_usage()
{
  fprintf(stdout,
"usage:\n"
" --host, -h    default host is localhost\n"
" --port, -p    default port is 8088\n"
" --verbose, -v\n"
  );
  exit(255);
}

int main(int argc, char** argv)
{
  int opt;
  int option_index = 0;
  static struct option long_options[] = {
    {"root",    required_argument, 0, 'r'},
    {"port",    required_argument, 0, 'p'},
    {"verbose", no_argument,       0, 'v'},
    {0,         0,                 0,  0}
  };

  struct sockaddr_in clientaddr;
  socklen_t addrlen;
  int slot = 0;

  // Initalize root path and listen port.
  initialize_environments((void**)&root_path, (void**)&port);

  while((opt = getopt_long(argc, argv, "r:p:v",
                           long_options, &option_index)) != -1) {
    switch(opt) {
    case 0:
      break;
    case 'r':
      grealloc(root_path, strlen(optarg));
      strcpy(root_path, optarg);
      break;
    case 'p':
      grealloc(port, strlen(optarg));
      strcpy(port, optarg);
      break;
    case 'v':
      break;
    case '?':
      print_usage();
    default:
      break;
    }
  }

  setup_signals();

  // Start server
  printf("Server starts at localhost:%s\nroot path is %s\n", port, root_path);

  // Initialize clients.
  for(int i=0; i<MAX_CONN; i++)
    clients[i] = -1;
  start_server();

  // Accept connections
  while(1)
  {
    addrlen = sizeof(clientaddr);
    clients[slot] = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
    
    if(clients[slot] < 0 ) {
      fprintf(stderr, "accpet error\n");
    }
    else {
      if(fork() == 0) {
        printf("Accepted!\n");
        respond(slot);
        exit(0);
      }
    }

    while(clients[slot] != -1) slot = (slot+1) % MAX_CONN;
  }

  cleanup_environments();

  exit(0);
}

void initialize_environments(void** root_path, void** port)
{
  *root_path = (char*)gmalloc(MAX_BUFFER);
  memset(*root_path, 0, MAX_BUFFER);
  *port = (char*)gmalloc(MAX_BUFFER);
  memset(*port, 0, MAX_BUFFER);
  strcpy(*root_path, getenv("PWD"));
  strcpy(*port, "8080"); 
}

void cleanup_environments()
{
  if(root_path != NULL)
    free(root_path);
  if(port != NULL)
    free(port);
}

void start_server()
{
  struct addrinfo hints, *res, *p;

  // get addrinfo for host
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if(getaddrinfo(NULL, port, &hints, &res) != 0) {
    fprintf(stderr, "getaddrinfo() error\n");
    exit(-1);
  }
  // bind socket
  for(p=res; p!=NULL; p=p->ai_next) {
    listenfd = socket(p->ai_family, p->ai_socktype, 0);
    if(listenfd == -1) continue;
    if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
  }

  if(p == NULL) {
    fprintf(stderr, "bind error\n");
    exit(-1);
  }

  freeaddrinfo(res);

  // Listening for incoming connections
  if(listen(listenfd, 1000 * 1000) != 0) {
    fprintf(stderr, "listen error\n");
    exit(-1);
  }
}

void respond(int n)
{
  char msg[MAX_MSG_LENGTH], *req_line[3], data_to_send[MAX_BUFFER], \
    path[MAX_MSG_LENGTH];
  int rcvd, fd, bytes_read;

  memset((void*)msg, 0, MAX_MSG_LENGTH);

  rcvd = recv(clients[n], msg, MAX_MSG_LENGTH, 0);
  if (rcvd < 0)
  {
    fprintf(stderr, "recv() error\n");
  }
  else if (rcvd == 0)
  {
    fprintf(stderr, "Client disconnected unexpectedly.\n");
  }
  else
  {
    printf("%s\n", msg);
    req_line[0] = strtok(msg, " \t\n");
    if (strncmp(req_line[0], "GET\0", 4) == 0 )
    {
      req_line[1] = strtok(NULL, " \t");
      req_line[2] = strtok(NULL, " \t\n");

      if (strncmp(req_line[2], "HTTP/1.0", 8) != 0 && strncmp(req_line[2], "HTTP/1.1", 8) != 0)
      {
        write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
      }
      else
      {
        if (strncmp(req_line[1], "/\0", 2) == 0) {
          req_line[1] = "/index.html";
        }
        strcpy(path, root_path);
        strcpy(&path[strlen(root_path)], req_line[1]);
        printf("file: %s\n", path);

        if ((fd=open(path, O_RDONLY)) != -1)
        {
          send(clients[n], "HTTP/1.0 200 OK\n", 17, 0);
          send(clients[n], "Content-Type: text/html\n\n", 25, 0);
          while((bytes_read=read(fd, data_to_send, MAX_BUFFER)) > 0)
          {
            write(clients[n], data_to_send, bytes_read);
          }
          close(fd);
        }
        else
        {
          write(clients[n], "HTTP/1.0 404 Not Found\n", 23);
        }
      }
    }
  }

  // Close socket
  shutdown(clients[n], SHUT_RDWR);
  close(clients[n]);
  clients[n] = -1;
}

/*
 * Functions used to bind customized signal handler with system signals
 */
void setup_signals()
{
  connect_signal(SIGINT, end_me);
  connect_signal(SIGQUIT, end_me);
  connect_signal(SIGTERM, end_me);
  connect_signal(SIGHUP, SIG_IGN);
}

void connect_signal(int signum, void (*func) (int))
{
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = func;
  sigemptyset(&sa.sa_mask);
  if (sigaction(signum, &sa, NULL) < 0) {
    fprintf(stderr, "sigaction() error!");
  }
}

void end_me(int signum)
{
  printf("Shutting down the server...\n");
  exit(0);
}
