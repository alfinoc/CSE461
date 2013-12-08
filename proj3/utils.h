#ifndef UTILS_HEADER
#define UTILS_HEADER

#include <sys/socket.h>

typedef struct sockaddr_in* sockaddr_t;

int send_tcp(char* mesg, int len, int sockfd);
int recieve_tcp(char* buf, int buf_len, int sockfd, int timeout);

int send_udp(char* mesg, int len, sockaddr_t sock_addr, int sockfd);
int recieve_udp(char* buf, int buf_len, sockaddr_t sock_addr, int sockfd, int timeout);

#endif
