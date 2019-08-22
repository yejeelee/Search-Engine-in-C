// Yejee Jenny Lee
// April 20, 2019

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "includes/QueryProtocol.h"

char *port_string = "1500";
unsigned short int port;
char *ip = "127.0.0.1";

#define BUFFER_SIZE 1000
#define QUERY_SIZE 100
char response[BUFFER_SIZE];

int openSocket();
struct addrinfo* createAddrInfo(char *port_string);
int connectSocket(int socket_fd, struct addrinfo *infoptr);
void doQueryProtocol(int socket_fd, char* query);


void doQueryProtocol(int socket_fd, char* query) {
  char *buffer = query;
  printf("SENDING Query to Server: %s \n", buffer);
  printf("\n Connected to movie server.\n");
  write(socket_fd, buffer, strlen(buffer));

  int len = read(socket_fd, response, BUFFER_SIZE - 1);
  response[len] = '\0';
  int responseNum = atoi(response);
  if (responseNum == 0) {
    printf("This term was not found in movie server.\n");
  } else {
    printf("received num of response : %d\n", responseNum);
  }
  for (int i = 0; i < responseNum; i++) {
    SendAck(socket_fd);
    len = read(socket_fd, response, BUFFER_SIZE - 1);
    response[len] = '\0';
    printf("%s\n", response);
  }
  SendAck(socket_fd);
  len = read(socket_fd, response, BUFFER_SIZE - 1);
  response[len] = '\0';

  CheckGoodbye(response);
}

struct addrinfo* createAddrInfo(char *port_string) {
struct addrinfo hints, *infoptr;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  int result = getaddrinfo(NULL, port_string, &hints, &infoptr);
  if (result != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
    exit(1);
  }
  return infoptr;
}

int openSocket() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("ERROR\n");
    exit(1);
  } else {
    printf("Socket opened\n");
  }
  return socket_fd;
}

int connectSocket(int socket_fd, struct addrinfo *infoptr) {
  if (connect(socket_fd, infoptr->ai_addr, infoptr->ai_addrlen) == -1) {
    freeaddrinfo(infoptr);
    perror("Cannot connect to Server\n");
    exit(2);
  }
  return 0;
}

void RunQuery(char *query) {
  // Find the address
  struct addrinfo *infoptr = createAddrInfo(port_string);

  // Create the socket
  int socket_fd = openSocket();

  // Connect to the server
  connectSocket(socket_fd, infoptr);
  freeaddrinfo(infoptr);
  int len = read(socket_fd, response, BUFFER_SIZE - 1);
  response[len] = '\0';
  if (CheckAck(response) == -1) {
    freeaddrinfo(infoptr);
    return;
  }
  // Do the query-protocol
  doQueryProtocol(socket_fd, query);
  // Close the connection
  close(socket_fd);
  return;
}

void RunPrompt() {
  char input[QUERY_SIZE];
  while (1) {
    printf("Enter a term to search for, or q to quit: ");
    scanf("%s", input);

    printf("input was: %s\n", input);

    if (strlen(input) == 1) {
      if (input[0] == 'q') {
        printf("Thanks for playing! \n");
        return;
      }
    }
    printf("\n\n");
    RunQuery(input);
  }
}

int main(int argc, char **argv) {
  // Check/get arguments
  if (argc != 3) {
    printf("input format: ./queryclient [ipaddress] [port]\n");
    return 1;
  }
  printf("Instruction:\n");
  printf("Search for movies in the movie server by entering a term.\n");
  // Get info from user
  ip = argv[1];
  port_string = argv[2];
  // Run Query
  RunPrompt();
  return 0;
}
