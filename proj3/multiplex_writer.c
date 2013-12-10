#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#include "utils.h"
#include "queue.h"
#include "multiplex_standards.h"
#include "multiplex_writer.h"

struct send_arg {
  struct queue* queue;
  char* host;
  int port;
};

void multi_send(char* buf, int len, char* host, int port, int num_conn);
void* queue_send(void*);
void build_queue(char* buf, int len, struct queue* q);

int main(int argc, char** argv) {

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <host> <port> <number of connections>\n", argv[0]);
    exit(1);
  }

  char* example_buffer = "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  int buf_len = 447;
    
  multi_send(example_buffer, buf_len, argv[1], atoi(argv[2]), atoi(argv[3]));
}

// spins up 'num_conn' threads, which will send the 'len' length contents
// of 'buf' to destination 'host':'port' on 'num_conn' TCP connections
void multi_send(char* buf, int len, char* host, int port, int num_conn) {
  // prepare common argument for all threads
  struct send_arg args;
  args.host = host;
  args.port = port;

  // prepare and build queue from buffer
  struct queue* blocks = allocate_queue();
  args.queue = blocks;
  build_queue(buf, len, blocks);

  // spin up all 'num_conn' threads
  pthread_t pthread;
  for (int i = 0; i < num_conn; i++)
    pthread_create(&pthread, NULL, queue_send, (void*) &args);
  
  while (true) ;
}

// opens a TCP connection to host on port, and sends from 'q' until
// 'q' is empty, returning when this is the case
void* queue_send(void* args) {
  // extract fields associated with host address and shared queue
  struct send_arg* extr = (struct send_arg*) args;
  struct queue* q = extr->queue;
  char* host = extr->host;
  int port = extr->port;

  // set up file descriptor
  struct sockaddr_in sa;
  sockaddr_t servaddr = &sa;
  int sockfd;
  open_tcp(host, port, servaddr, &sockfd);

  // write until the queue is empty
  while (!is_empty(q)) {
    char* block;
    int block_size = dequeue(q, &block);
    send_tcp(block, block_size, sockfd);
  }
  close(sockfd);
  return NULL;
}

// fills the queue pointed to by 'q' with pointers to segments of
// 'buf' of length BLOCK_SIZE. overall 'buf' is size 'len' bytes
void build_queue(char* buf, int len, struct queue* blocks) {
  int i;
  for (i = 0; i < len; i += BLOCK_SIZE)
    enqueue(blocks, &(buf[i]), i + BLOCK_SIZE > len ? len - i : BLOCK_SIZE);
}

