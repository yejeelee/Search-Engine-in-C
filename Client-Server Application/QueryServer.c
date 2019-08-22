// Yejee Jenny Lee
// April 20, 2019

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>


#include "QueryProtocol.h"
#include "MovieSet.h"
#include "MovieIndex.h"
#include "DocIdMap.h"
#include "htll/Hashtable.h"
#include "QueryProcessor.h"
#include "FileParser.h"
#include "FileCrawler.h"

DocIdMap docs;
Index docIndex;

#define BUFFER_SIZE 1000
#define SEARCH_RESULT_LENGTH 1500
char movieSearchResult[SEARCH_RESULT_LENGTH];
int socket_fd;

struct addrinfo* createAddrInfo(char *port_string);
int openSocket();
int bindSocket(struct addrinfo *infoptr);
int listenSocket();
int HandleConnections(int socket_fd);
int Cleanup();

void sigint_handler(int sig) {
  write(0, "Exit signal sent. Cleaning up...\n", 34);
  Cleanup();
  exit(0);
}


void Setup(char *dir) {
  printf("Crawling directory tree starting at: %s\n", dir);
  // Create a DocIdMap
  docs = CreateDocIdMap();
  CrawlFilesToMap(dir, docs);
  printf("Crawled %d files.\n", NumElemsInHashtable(docs));

  // Create the index
  docIndex = CreateIndex();

  // Index the files
  printf("Parsing and indexing files...\n");
  ParseTheFiles(docs, docIndex);
  printf("%d entries in the index.\n", NumElemsInHashtable(docIndex->ht));
}

int Cleanup() {
  printf("SERVER CLOSING...\n");
  close(socket_fd);
  DestroyOffsetIndex(docIndex);
  DestroyDocIdMap(docs);
  return 0;
}

int HandleConnections(int socket_fd) {
  printf("==============\n");
  printf("Waiting for the connection...\n");
  int client_fd = accept(socket_fd, NULL, NULL);

  printf("<<< Connection made: client_fd=%d >>>\n", client_fd);
  SendAck(client_fd);
  char buffer[BUFFER_SIZE];
  int len = read(client_fd, buffer, BUFFER_SIZE - 1);
  buffer[len] = '\0';
  printf("Client asked for word: %s\n", buffer);
  int NumResp;
  SearchResultIter iter = FindMovies(docIndex, buffer);
  SearchResult data = (SearchResult)malloc(sizeof(*data));
  if (iter == NULL) {
    NumResp = 0;
    printf("This term is not found! \n");
  } else {
    NumResp = NumResultsInIter(iter);
  }
  sprintf(buffer, "%d", NumResp);
  printf("SENDING number of response: %s \n\n", buffer);
  write(client_fd, buffer, strlen(buffer));
  int i = 0;
  while (i < NumResp) {
    char resp[BUFFER_SIZE];
    int len = read(client_fd, resp, BUFFER_SIZE - 1);
    resp[len] = '\0';
    if (CheckAck(resp) == -1) {
      return 1;
    }
    SearchResultGet(iter, data);
    CopyRowFromFile(data, docs, buffer);
    write(client_fd, buffer, strlen(buffer));
    SearchResultNext(iter);
    i++;
  }
  len = read(client_fd, buffer, sizeof(buffer) - 1);
  buffer[len] = '\0';
  if (CheckAck(buffer) == -1) {
    return 1;
  }
  free(data);
  if (iter != NULL) {
    DestroySearchResultIter(iter);
    printf("Destroying search result iter.\n");
  }
  printf("sending GOODBYE to client=%d. \n", client_fd);
  SendGoodbye(client_fd);
  close(client_fd);
  printf("<<< CONNECTION CLOSED. >>> \n");
  printf("==============\n\n");
  return 0;
}

int openSocket() {
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("Creating socket failed! \n");
    exit(1);
  }
  return 0;
}

int bindSocket(struct addrinfo *infoptr) {
  if (bind(socket_fd, infoptr->ai_addr, infoptr->ai_addrlen) != 0) {
    perror("bind() ");
    exit(1);
  }
  printf("bind socket successful. \n");
  return 0;
}

int listenSocket() {
  if (listen(socket_fd, 10) != 0) {
    perror("listen() ");
    exit(1);
  }
  printf("listening to client.\n");
  return 0;
}

struct addrinfo* createAddrInfo(char *port_string) {
struct addrinfo hints, *infoptr;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  // Setup graceful exit
  struct sigaction kill;

  kill.sa_handler = sigint_handler;
  kill.sa_flags = 0;  // or SA_RESTART
  sigemptyset(&kill.sa_mask);

  if (sigaction(SIGINT, &kill, NULL) == -1) {
    perror("sigaction");
    freeaddrinfo(infoptr);
    exit(1);
  }
  int result = getaddrinfo(NULL, port_string, &hints, &infoptr);
  if (result != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
    exit(1);
  }
  return infoptr;
}

int main(int argc, char **argv) {
  // Get args
  if (argc != 3) {
    printf("input should be: ./queryserver [datadir] [port]\n");
    return 1;
  }
  char *datadir = argv[1];
  char *port_string = argv[2];

  // Step 1: get address/port info to open
  struct addrinfo *infoptr = createAddrInfo(port_string);
  Setup(datadir);

  // Step 2: Open socket
  openSocket();

  // Step 3: Bind socket
  bindSocket(infoptr);

  // Step 4: Listen on the socket
  listenSocket();

  struct sockaddr_in *result_addr = (struct sockaddr_in *) infoptr->ai_addr;
  printf("Listening on file descriptor %d, port %d\n", socket_fd,
          ntohs(result_addr->sin_port));

  // Step 5: Handle clients that connect
  freeaddrinfo(infoptr);

  while (1) {
    HandleConnections(socket_fd);
  }
  // Step 6: Close the socket
  // Got Kill signal
  Cleanup();

  return 0;
}
