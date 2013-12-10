#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

#include "utils.h"
#include "queue.h"
#include "multiplex_standards.h"
#include "multiplex_writer.h"

void multi_send(struct queue* q, char* host, int port);
void build_queue(char* buf, int len, struct queue* q);

int main(int argc, char** argv) {

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <host> <port> <number of connections>\n", argv[0]);
    exit(1);
  }

  char* host = argv[1];
  uint32_t port = atoi(argv[2]);
  //int num_conn = atoi(argv[3]);


  char* example_buffer = "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  int buf_len = 447;
    
  struct queue* blocks = allocate_queue();
  build_queue(example_buffer, buf_len, blocks);

  multi_send(blocks, host, port);
}

// opens a tcp connection to host on port, and sends from 'q' until
// 'q' is empty, returning when this is the case
void multi_send(struct queue* q, char* host, int port) {
  struct sockaddr_in sa;
  sockaddr_t servaddr = &sa;
  int sockfd;

  open_tcp(host, port, servaddr, &sockfd);

  while (!is_empty(q)) {
    char* block;
    int block_size = dequeue(q, &block);
    send_tcp(block, block_size, sockfd);
  }
}

// fills the queue pointed to by 'q' with pointers to segments of
// 'buf' of length BLOCK_SIZE. overall 'buf' is size 'len' bytes
void build_queue(char* buf, int len, struct queue* blocks) {
  int i;
  for (i = 0; i < len; i += BLOCK_SIZE)
    enqueue(blocks, &(buf[i]), i + BLOCK_SIZE > len ? len - i : BLOCK_SIZE);
}

