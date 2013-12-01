#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>

#define MAX_CONN 2

struct client_info {
  int num_connections;
  struct sockaddr_in client_addr;
};

typedef struct sockaddr_in* sockaddr_t;

int validate_init_req(struct client_info* packet, int len);
int send_tcp(char* mesg, int len, int sockfd);
int recieve_udp(char* buf, int buf_len, sockaddr_t sock_addr, int sockfd, int timeout);
void* serve_client(void* arg);

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s PORT\n", argv[0]);
    exit(1);
  }

  int first_port = atoi(argv[1]);

  // set up the perminant udp server
  struct sockaddr_in main_servaddr;
  int main_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  bzero(&main_servaddr, sizeof(main_servaddr));
  main_servaddr.sin_family = AF_INET;
  main_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  main_servaddr.sin_port = htons(first_port);
  bind(main_sockfd, (struct sockaddr *)&main_servaddr, sizeof(main_servaddr));

  struct sockaddr_in client_addr;
  int rec_size;
  struct client_info* arg;
  pthread_t pthread;

  while (1) {
    printf("waiting for incoming udp packet...\n");
    
    // prepare arg for new client thread
    arg = (struct client_info*) malloc(sizeof(struct client_info));
    if (arg == NULL) {
      fprintf(stderr, "error with malloc, errno %d\n", errno);
      exit(errno);
    }

    rec_size = recieve_udp((void*) &(arg->num_connections), sizeof(int),
                           (sockaddr_t) &client_addr, main_sockfd, 0);
    
    // validate
    if (rec_size != sizeof(int) || arg->num_connections > MAX_CONN
       || arg->num_connections < 0) {
      perror("malformed initialization packet\n");
      continue;
    }

    arg->client_addr = client_addr;

    pthread_create(&pthread, NULL, serve_client, (void*) arg);
  }
}

int send_udp(char* mesg, int len, sockaddr_t sock_addr, int sockfd) {
  int ret = sendto(sockfd, (void*)mesg, (size_t)len, 0,
                   (struct sockaddr *) sock_addr, sizeof(*sock_addr));
  if (ret == -1) {
    fprintf(stderr, "send returned error: %s\n", strerror(errno));
  }
  return ret;
}

int recieve_udp(char* buf, int buf_len, sockaddr_t sock_addr, int sockfd, int timeout) {
  if (timeout != 0) {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      perror("Error");
    }
  }

  socklen_t socksize = sizeof(*sock_addr);
  return recvfrom(sockfd, buf, buf_len, 0, (struct sockaddr*) sock_addr, &socksize);
}

void* serve_client(void* arg) {
  struct client_info* client = (struct client_info*) arg;

  // set up connections to each of the provided mac addresses


  // acknowledge receipt with list of ports for open TCP connections
  

  // handle connection
  return NULL;
}
