#ifndef UTILS_HEADER
#define UTILS_HEADER

int send_tcp(char* mesg, int len, int sockfd);
int recieve_tcp(char* buf, int buf_len, int sockfd, int timeout);

int send_udp(char* mesg, int len, sockaddr_t sock_addr, int sockfd);
int recieve_udp(char* buf, int buf_len, sockaddr_t sock_addr, int sockfd, int timeout);

#endif
