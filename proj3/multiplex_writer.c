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
void fill_queue(char* buf, int len, struct queue* q);
int listen_for_blocks(struct mp_writer_init* arg);
int min(int n, int m);

int next_tag = 0;

void *multiplex_writer_init(void* arg) {
  struct mp_writer_init* init_params = (struct mp_writer_init*) arg;

  listen_for_blocks(init_params);

  fprintf(stderr, "multiplex_writer_init called on file descriptor %d\n",
	  init_params->client_fd);
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

    send_tcp(buf, buf_len, arg->client_fd);
    free(buf);
  }
}

void write_block_to_queue(struct queue* queue, char* block, int block_size) {
  pthread_mutex_lock(&queue->mutex);

  enqueue(queue, block, block_size);

  pthread_cond_signal(&queue->cv);
  pthread_mutex_unlock(&queue->mutex);
}

// split up the buffer into blocks and distribute them among the queues
void multiplex_write_queues(struct queue** queues, int num_queues,
			    char* buffer, int buf_len) {
  int unif_size = min(BLOCK_SIZE, buf_len / num_queues);

  for (int i = 0; i < buf_len; i += unif_size) {
    int actual_size = min(unif_size, buf_len - i);
    char* headered_block = (char*) malloc(sizeof(struct data_block)
					  + actual_size);

    if (headered_block == NULL) {
      fprintf(stderr, "Could not malloc new block in multiplex_writer: errno %d\n", errno);
      exit(1);
    }

    struct data_block header;
    header.block_size = actual_size;
    header.seq_number = i;
    header.tag = next_tag++;

    memcpy((void*) headered_block, &header, sizeof(struct data_block));
    memcpy((void*) (headered_block + sizeof(struct data_block)),
	   (void*) &(buffer[i]), actual_size);
    enqueue(queues[i % num_queues], headered_block,
	    sizeof(struct data_block) + actual_size);
  }
}

// returns the minimum of 'n' and 'm'
int min(int n, int m) {
  return n > m ? m : n;
}


