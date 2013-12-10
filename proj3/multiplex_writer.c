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
int listen_for_blocks(struct mp_writer_init* arg);

void *multiplex_writer_init(void* arg) {
  struct mp_writer_init* init_params = (struct mp_writer_init*) arg;

  listen_for_blocks(init_params);

  fprintf(stderr, "multiplex_writer_init called on file descriptor %d\n", init_params->client_fd);
  return NULL;
}

int listen_for_blocks(struct mp_writer_init* arg) {
  while (true) {
    pthread_mutex_lock(&arg->queue->mutex);
    while (is_empty(arg->queue)) {
      pthread_cond_wait(&arg->queue->cv, &arg->queue->mutex);
    }
    
    char* buf;
    int buf_len = dequeue(arg->queue, &buf);
    
    pthread_mutex_unlock(&arg->queue->mutex);

    // send block (what's put on the queue will only be data, so we
    // should handle the header in the module)
  }
}

void write_block_to_queue(struct queue* queue, char* block, int block_size) {
  pthread_mutex_lock(&queue->mutex);

  enqueue(queue, block, block_size);

  pthread_cond_signal(&queue->cv);
  pthread_mutex_unlock(&queue->mutex);
}

// split up the buffer into blocks and distribute them among the queues
void multiplex_write_queues(struct queue** queues, char* buffer, int buf_len) {
  
}

// I had to make this "main2" to link with the application code
int main2(int argc, char** argv) {

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <host> <port> <number of connections>\n", argv[0]);
    exit(1);
  }

  char* example_buffer = "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  int buf_len = 447;
    
  multi_send(example_buffer, buf_len, argv[1], atoi(argv[2]), atoi(argv[3]));

  return 0;
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

  // spin up all 'num_conn' threads and block until they each finish up
  pthread_t pid[num_conn];
  for (int i = 0; i < num_conn; i++)
    pthread_create(&pid[i], NULL, queue_send, (void*) &args);
  for (int i = 0; i < num_conn; i++)
    pthread_join(pid[i], NULL);
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

  open_tcp(host, port, servaddr, &sockfd, NULL);

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

