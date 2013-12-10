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
#include "multiplex_standards.h"
#include "multiplex_writer.h"

struct queue {
  struct queue_node* head;
  struct queue_node* tail;
};

struct queue_node {
  char* buf;
  int buf_len;
  struct queue_node* prev;
  struct queue_node* next;
};

void multi_send(struct queue* q, char* host, int port);
void build_queue(char* buf, int len, struct queue* q);
struct queue* allocate_queue();
void free_queue(struct queue* queue);
int dequeue(struct queue* queue, char** buf_ptr);
void enqueue(struct queue* queue, char* buf, int buf_len);
int is_empty(struct queue* queue);
void print_queue(struct queue* queue);

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

// allocates pointer to a newly allocated queue, exiting on malloc failure
struct queue* allocate_queue() {
  struct queue* q = (struct queue*) malloc(sizeof(struct queue));
  if (q == NULL) {
    fprintf(stderr, "error with malloc, errno %d\n", errno);
    exit(errno);
  }
  q->head = NULL;
  q->tail = NULL;
  return q;
}

// frees the queue and every remaining node. does not call free on the
// buffers stored inside the queue
void free_queue(struct queue* queue) {
  while (queue->head != NULL) {
    struct queue_node* temp = queue->head;
    queue->head = queue->head->next;
    free(temp);
  }
  free(queue);
}

// removes the first element in the queue, providing a pointer to the
// buffer via 'buf_ptr', returning the size of the buffer. returns
// NULL if the queue is empty
int dequeue(struct queue* queue, char** buf_ptr) {
  *buf_ptr = queue->tail->buf;
  int size = queue->tail->buf_len;
  struct queue_node* old_tail = queue->tail;
  queue->tail = queue->tail->prev;

  if (queue->tail == NULL)
    queue->head = NULL;
  else
    queue->tail->next = NULL;
  free(old_tail);
  return size;
}

// adds 'buf' to the queue.
void enqueue(struct queue* queue, char* buf, int buf_len) {
  struct queue_node* new_head = (struct queue_node*)
    malloc(sizeof(struct queue_node));
  if (new_head == NULL) {
    fprintf(stderr, "error with malloc, errno %d\n", errno);
    exit(errno);
  }
  int was_empty = (queue->head == NULL);

  new_head->buf = buf;
  new_head->buf_len = buf_len;
  new_head->next = queue->head;
  new_head->prev = NULL;
  
  queue->head = new_head;
  if (!was_empty)
    queue->head->next->prev = queue->head;
  else
    queue->tail = queue->head;
}

// returns 1 if the queue is empty, 0 otherwise
int is_empty(struct queue* queue) {
  return queue->head == NULL;
}

// prints the contents of the 'queue', one buffer per line
void print_queue(struct queue* queue) {
  struct queue_node* curr = queue->head;
  printf("QUEUE START\n");
  //  printf("   tail: %p\n", queue->tail);
  //  printf("   head: %p\n", queue->head);
  while (curr != NULL) {
    for (int i = 0; i < curr->buf_len; i++)
      printf("%c ", curr->buf[i]);
    printf("(%p)\n", curr);
    //    printf("   next: %p\n", curr->next);
    //    printf("   prev: %p\n", curr->prev);
    curr = curr->next;
  }
  printf("QUEUE END\n");
}
