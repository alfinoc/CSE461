#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include "queue.h"

// allocates pointer to a newly allocated queue, exiting on malloc failure
struct queue* allocate_queue() {
  struct queue* q = (struct queue*) malloc(sizeof(struct queue));
  if (q == NULL) {
    fprintf(stderr, "error with malloc, errno %d\n", errno);
    exit(errno);
  }
  q->head = NULL;
  q->tail = NULL;

  // initialize lock
  sem_init(&q->wrt, 0, 1);
  pthread_mutex_init(&q->mutex, 0);
  pthread_cond_init(&q->cv, 0);

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
  sem_wait(&queue->wrt);
  if (queue->head == NULL) {
    sem_post(&queue->wrt);
    return 0;
  }

  *buf_ptr = queue->tail->buf;
  int size = queue->tail->buf_len;
  struct queue_node* old_tail = queue->tail;
  queue->tail = queue->tail->prev;

  if (queue->tail == NULL)
    queue->head = NULL;
  else
    queue->tail->next = NULL;
  free(old_tail);

  sem_post(&queue->wrt);
  return size;
}


// adds 'buf' to the queue.
void enqueue(struct queue* queue, char* buf, int buf_len) {
  sem_wait(&queue->wrt);

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

  sem_post(&queue->wrt);
}

// returns 1 if the queue is empty, 0 otherwise
int is_empty(struct queue* queue) {
  sem_wait(&queue->wrt);
  int empty = queue->head == NULL;
  sem_post(&queue->wrt);
  return empty;
}

// prints the contents of the 'queue', one buffer per line
void print_queue(struct queue* queue) {
  struct queue_node* curr = queue->head;
  printf("QUEUE START\n");
  while (curr != NULL) {
    for (int i = 0; i < curr->buf_len; i++)
      printf("%c ", curr->buf[i]);
    printf("(%p)\n", curr);
    curr = curr->next;
  }
  printf("QUEUE END\n");
}

