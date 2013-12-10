#ifndef QUEUE_HEADER
#define QUEUE_HEADER

#include <semaphore.h>

struct queue {
  struct queue_node* head;
  struct queue_node* tail;
  sem_t wrt;
};

struct queue_node {
  char* buf;
  int buf_len;
  struct queue_node* prev;
  struct queue_node* next;
};

struct queue* allocate_queue();
void free_queue(struct queue* queue);
int dequeue(struct queue* queue, char** buf_ptr);
void enqueue(struct queue* queue, char* buf, int buf_len);
int is_empty(struct queue* queue);
void print_queue(struct queue* queue);

#endif
