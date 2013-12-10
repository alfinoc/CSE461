#include "utils.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

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

int recieve_tcp(char* buf, int buf_len, int sockfd, int timeout) {
  if (timeout != 0) {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      perror("Error");
    }
  }

  int n = read(sockfd, buf, buf_len);

  return n;
}

int send_tcp(char* mesg, int len, int sockfd) {
  int ret = send(sockfd, mesg, len, 0);

  if (ret == -1) {
    fprintf(stderr, "send returned error: %s\n", strerror(errno));
  }
  return ret;
}

void open_udp(char* addr, uint32_t port, sockaddr_t servaddr, int* sockfd) {
  *sockfd = socket(AF_INET,SOCK_DGRAM,0);

  bzero(servaddr,sizeof(struct sockaddr_in));
  servaddr->sin_family = AF_INET;
  servaddr->sin_addr.s_addr = inet_addr(addr);
  servaddr->sin_port=htons(port);
}

int open_tcp(char* addr, uint32_t port, sockaddr_t servaddr, int* sockfd, char* nic_addr) {
  *sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  bzero(servaddr,sizeof(struct sockaddr_in));
  servaddr->sin_family = AF_INET;
  servaddr->sin_addr.s_addr = inet_addr(addr);
  servaddr->sin_port=htons(port);

  fprintf(stderr, "attemping to connect on port %x...", port);

  if (nic_addr != NULL) {
    struct sockaddr_in client_addr;
    memset(&client_addr, '0', sizeof(client_addr));
    
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(5000); //shouldn't matter i don't think
    if(inet_pton(AF_INET, nic_addr, &client_addr.sin_addr)<=0) {
      printf("\n inet_pton error occured on client bind\n");
      return -1;
    }

    bind(*sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr));
  }

  int ret = -1;
  do {
    ret = connect(*sockfd, (struct sockaddr *) servaddr, sizeof(struct sockaddr_in));

    if (ret == -1)
      fprintf(stderr, "error: %s... ", strerror(errno));
  } while (ret == -1);
  fprintf(stderr, "connected.\n");

  return ret;
}
