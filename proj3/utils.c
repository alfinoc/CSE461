#include "utils.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>

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

  int n = recv(sockfd, buf, buf_len, 0);

  return n;
}

int send_tcp(char* mesg, int len, int sockfd) {
  int ret = send(sockfd, mesg, len, 0);

  if (ret == -1) {
    fprintf(stderr, "send returned error: %s\n", strerror(errno));
  }
  return ret;
}
