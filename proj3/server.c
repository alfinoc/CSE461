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

#define MAX_CONN 5

struct client_info {
  int num_conn;
  struct sockaddr_in sock_addr;
  int sock_fd;
};

typedef struct sockaddr_in* sockaddr_t;

int send_udp(char* mesg, int len, sockaddr_t sock_addr, int sockfd);
int recieve_udp(char* buf, int buf_len, sockaddr_t sock_addr, int sockfd, int timeout);
int bind_random_port(struct sockaddr_in*, int type);
void* serve_client(void* arg);

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s PORT\n", argv[0]);
    exit(1);
  }

  // seed randomness
  srand(time(NULL));

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

    rec_size = recieve_udp((void*) &(arg->num_conn), sizeof(int),
                           (sockaddr_t) &client_addr, main_sockfd, 0);
    arg->num_conn = ntohl(arg->num_conn);

    // validate
    if (rec_size != sizeof(int) || arg->num_conn > MAX_CONN
       || arg->num_conn < 0) {
      perror("malformed initialization packet\n");
      continue;
    }

    arg->sock_addr = client_addr;
    arg->sock_fd = main_sockfd;

    pthread_create(&pthread, NULL, serve_client, (void*) arg);
  }
}

void* serve_client(void* arg) {
  struct client_info* init = (struct client_info*) arg;
  int* tcp_fds = (int*) malloc(sizeof(int) * init->num_conn);
  uint32_t ack_buf[init->num_conn];
  printf("requested number of connections: %d\n  ports:\n", init->num_conn);

  // set up tcp connections, store each in tcp_conns
  for (int i = 0; i < init->num_conn; i++) {
    struct sockaddr_in tcp_servaddr;
    tcp_fds[i] = bind_random_port(&tcp_servaddr, SOCK_STREAM);
    ack_buf[i] = htonl((uint32_t) ntohs(tcp_servaddr.sin_port));
    printf("    %d\n", tcp_servaddr.sin_port);
  }

  // acknowledge receipt with list of ports for open TCP connections
  send_udp((char*) ack_buf, sizeof(int) * init->num_conn, &(init->sock_addr),
	   init->sock_fd);

  // accept connections -> new threads?

  return NULL;
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

int bind_random_port(struct sockaddr_in* servaddr, int type) {
  uint16_t port;
  uint16_t port_range = 65524 - 256;
  int sockfd;
  int ret;

  sockfd = socket(AF_INET, type, 0);
  bzero(servaddr, sizeof(struct sockaddr_in));
  servaddr->sin_family = AF_INET;
  servaddr->sin_addr.s_addr=htons(INADDR_ANY);

  do {
    port = (rand() % port_range) + 256;
    servaddr->sin_port = htons(port);
    ret = bind(sockfd, (struct sockaddr *) servaddr, sizeof(struct sockaddr_in));
  } while (ret == -1);

  return sockfd;
}
