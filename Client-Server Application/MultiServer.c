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
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "QueryProtocol.h"
#include "MovieSet.h"
#include "MovieIndex.h"
#include "DocIdMap.h"
#include "htll/Hashtable.h"
#include "QueryProcessor.h"
#include "FileParser.h"
#include "FileCrawler.h"

#define BUFFER_SIZE 1000
char buffer[BUFFER_SIZE];

int manageSocket(char *port_string);
int openSocket();
int listenSocket(int socket_fd);
int readACK(char* buffer, int client_fd);
int checkACK(char* buffer, int client_fd);
int listenFileDescriptor(struct addrinfo *infoptr, int socket_fd);
int Cleanup();

DocIdMap docs;
Index docIndex;

#define SEARCH_RESULT_LENGTH 1500
char movieSearchResult[SEARCH_RESULT_LENGTH];


void sigchld_handler(int s) {
  write(0, "Handling zombies...\n", 20);
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
}

void sigint_handler(int sig) {
  write(0, "Ahhh! SIGINT!\n", 14);
  Cleanup();
  exit(0);
}

int readACK(char* buffer, int client_fd) {
  int len = read(client_fd, buffer, sizeof(buffer) - 1);
  buffer[len] = '\0';
  return 0;
}

int checkACK(char* buffer, int client_fd) {
  if(readACK(buffer, client_fd) == 0) {
    if (CheckAck(buffer) == -1) {
      return 1;
    }
  }
  return 0;
}

int HandleConnections(int sock_fd) {
  // Step 5: Accept connection
  // Fork on every connection.
  printf("====================\n");
  printf("Waiting for the connection...\n");
  int client_fd = accept(sock_fd, NULL, NULL);
  printf("====================\n");
  printf("Connection made: client_fd=%d\n", client_fd);
  printf("====================\n");

  pid_t child_id = fork();
  if (child_id == -1) {
    perror("fork() ");
    exit(1);
  }
  if (child_id == 0) {  // in child
    close(sock_fd);
    SendAck(client_fd);
    readACK(buffer, client_fd);
    
    sleep(2);
    printf("**Recieved query = %s from client=%d. \n", buffer, client_fd);

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
    printf("**SENDING number of response: %s \n\n", buffer);
    write(client_fd, buffer, strlen(buffer));
    int i = 0;
    while (i < NumResp) {
      char resp[BUFFER_SIZE];
      checkACK(resp, client_fd);
      SearchResultGet(iter, data);
      CopyRowFromFile(data, docs, buffer);
      write(client_fd, buffer, strlen(buffer));
      SearchResultNext(iter);
      i++;
    }
    checkACK(buffer, client_fd);

    free(data);
    if (iter != NULL) {
      DestroySearchResultIter(iter);
      printf("====================\n");
      printf("Destroying search result iter.\n");
      printf("====================\n");
    }
    printf("**sending GOODBYE to client=%d. \n", client_fd);
    SendGoodbye(client_fd);
    close(client_fd);
    printf("<<< CONNECTION CLOSED. >>> \n");
    exit(0);
  } else {
    close(client_fd);
  }
  return 0;
}

void Setup(char *dir) {
  struct sigaction sa;

  sa.sa_handler = sigchld_handler;  // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  struct sigaction kill;

  kill.sa_handler = sigint_handler;
  kill.sa_flags = 0;  // or SA_RESTART
  sigemptyset(&kill.sa_mask);

  if (sigaction(SIGINT, &kill, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

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
  DestroyOffsetIndex(docIndex);
  DestroyDocIdMap(docs);
  return 0;
}

int manageSocket(char *port_string) {
  struct addrinfo hints, *infoptr, *total;

  // create address
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int result = getaddrinfo(NULL, port_string, &hints, &infoptr);
  if (result != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
    exit(1);
  }

  // open socket
  int socket_fd = openSocket();

  // bind socket
  int true = 1;
  for (total = infoptr; total != NULL; total = total -> ai_next) {
    if ((socket_fd = socket(total -> ai_family, total -> ai_socktype,
            total -> ai_protocol)) != 0) {
      perror("socket()");
    }    
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &true,
		   sizeof(int)) != 0) {
      perror("bind() in used...");
      exit(1);
    }
    if (bind(socket_fd, total->ai_addr, total->ai_addrlen) != 0) {
      perror("bind()");
      exit(1);
    }
  }
  printf("bind socket successful. \n");

  // listen socket
  listenSocket(socket_fd);

  listenFileDescriptor(infoptr, socket_fd);
  freeaddrinfo(infoptr);
  
  return socket_fd;
}



int listenFileDescriptor(struct addrinfo *infoptr, int socket_fd) {
  struct sockaddr_in *result_addr = (struct sockaddr_in*) infoptr->ai_addr;
  printf("Listening on file descriptor %d, port %d\n", socket_fd,
	 ntohs(result_addr->sin_port));
  return 0;
}

int openSocket() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("Creating socket failed! \n");
    exit(1);
  }
  return socket_fd;
}

int listenSocket(int socket_fd) {
  if (listen(socket_fd, 10) != 0) {
    perror("listen() ");
    exit(1);
  }
  printf("listening to client.\n");
  return 0;
}

int main(int argc, char **argv) {
  // Get args
  if (argc != 3) {
    printf("====================\n");
    printf("Input format: multiserver [datadir] [port]\n");
    printf("====================\n");
    return 1;
  }
  char *dir_to_crawl = argv[1];
  char *port_string = argv[2];

  if (access(dir_to_crawl, F_OK) == 0) {
    Setup(dir_to_crawl);
  } else {
    printf("====================\n");
    printf("Directory does not exist.\n");
    printf("====================\n");
    return 1;
  }
  
  // Get address and connect socket
  int socket_fd = manageSocket(port_string);

  // Handle the connections
  while (1) {
    HandleConnections(socket_fd);
  }
  // Got Kill signal
  close(socket_fd);
  Cleanup();
  return 0;
}
