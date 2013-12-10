#ifndef UTILS_HEADER
#define UTILS_HEADER

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

typedef struct sockaddr_in* sockaddr_t;

int send_tcp(char* mesg, int len, int sockfd);
int recieve_tcp(char* buf, int buf_len, int sockfd, int timeout);

int send_udp(char* mesg, int len, sockaddr_t sock_addr, int sockfd);
int recieve_udp(char* buf, int buf_len, sockaddr_t sock_addr, int sockfd, int timeout);

void open_udp(char* addr, uint32_t port, sockaddr_t servaddr, int* sockfd);
int open_tcp(char* addr, uint32_t port, sockaddr_t servaddr, int* sockfd, char* nic_addr);

#endif
