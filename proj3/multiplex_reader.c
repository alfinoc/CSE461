#include "queue.h"
#include "multiplex_reader.h"
#include "utils.h"
#include <stdbool.h>
#include "multiplex_standards.h"
#include "HashTable.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct buffer_info {
  char* buffer;
  uint32_t buffer_size;
  int tag;
  bool completed;
  bool sent;
  uint32_t n_written;
  int prev_tag;
  bool last_block;
  pthread_mutex_t mutex; 
  bool valid;
  int next_tag;
  struct queue* waiting_queue;
};


bool insert_dummy_buffer(HashTable ht, uint32_t tag, struct buffer_info** buffer,
			 uint32_t next_tag, struct queue* waiting_queue);
void listen_for_block(struct mp_reader_init* params);
void handle_block(HashTable ht, char* block, struct mp_reader_init* arg);
void handle_finished_buffer_prev(HashTable ht, HTKeyValue buffer_kv, struct mp_reader_init* arg);
void handle_finished_buffer_next(HashTable ht, HTKeyValue buffer_kv, struct mp_reader_init* arg);
//void send_buffer(void* buffer, uint32_t buffer_size, struct mp_reader_init* arg);
int recieve_tcp_block(char* buf, int buf_len, int sockfd, int timeout);

void *multiplex_reader_init(void *arg) {
  struct mp_reader_init* params = (struct mp_reader_init*) arg;
  
  listen_for_block(params);
  return NULL;
}

void listen_for_block(struct mp_reader_init* arg) {
  int max_size = BLOCK_SIZE + sizeof(struct data_block);
  int ret;
  while(true) {
    char *block = (char *)malloc(max_size);

    ret = recieve_tcp_block(block, max_size, arg->client_fd, 0);
    // 0 is EOF, the client probably closed the connection
    if (ret == -1) {
      fprintf(stderr, "error on read! panicc\n");
      return;
    }

    if (ret == 0) {
      fprintf(stderr, "closing fd %d\n", arg->client_fd);
      close(arg->client_fd);
      return;
    }
    handle_block(arg->ht, block, arg);
 }
}

// reads in a block at time, using the chunk_size field to know when to stop reading.
int recieve_tcp_block(char* buf, int buf_len, int sockfd, int timeout) {
  if (timeout != 0) {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      perror("Error");
    }
  }

  int ret;
  int index = 0 ;
  uint32_t block_size;
  // read in at least the first 4 bytes to learn the chunk size
  while (index < sizeof(block_size)) {
    ret = read(sockfd, &buf[index], sizeof(block_size) - index);
    if (ret == -1)
      fprintf(stderr, "read returned error (fd %d): %s\n", sockfd, strerror(errno));
    if (ret < 1)
      return ret;
    index += ret;
  }
  block_size = *(uint32_t*)buf;
  // read in the rest of the chunk
  while (index < block_size) {
    ret = read(sockfd, &buf[index], block_size - index);
    if (ret == -1)
      fprintf(stderr, "read returned error (fd %d): %s\n", sockfd, strerror(errno));
    if (ret < 1)
      return ret;
    index += ret;
  }
  return index;
}

bool insert_dummy_buffer(HashTable ht, uint32_t tag, struct buffer_info** buffer,
			 uint32_t next_tag, struct queue* waiting_queue) {
  *buffer = (struct buffer_info*) calloc(1, sizeof(struct buffer_info));
  
  pthread_mutex_init(&(*buffer)->mutex, NULL);
  (*buffer)->tag = tag;
  (*buffer)->valid = false;
  (*buffer)->next_tag = next_tag;
  (*buffer)->waiting_queue = waiting_queue;

  HTKeyValue buffer_kv;
  buffer_kv.value = (void *) *buffer;
  buffer_kv.key = tag;

  HTKeyValue dummy;
  if (InsertHashTable(ht, buffer_kv, &dummy) == 2) {
    InsertHashTable(ht, dummy, &buffer_kv); // put it back!
    //someone beat us to it!
    free(*buffer);

    *buffer = (struct buffer_info*) dummy.value;
    return false;
  }
 
  return true;
}

void handle_block(HashTable ht, char* block, struct mp_reader_init* arg) {
  int* head = ((int*) block + 1);
  if (*head == -1) {
    // initialize a new buffer
    struct init_block* init = (struct init_block*) block;
    HTKeyValue keyvalue;
    int found = LookupHashTable(ht, init->tag, &keyvalue);
    
    struct buffer_info* new_buf;

    if (!found) {
      new_buf = (struct buffer_info*) calloc(1, sizeof(struct buffer_info));
      pthread_mutex_init(&new_buf->mutex, NULL);
      new_buf->next_tag = NO_TAG;
    } else {
      new_buf = (struct buffer_info*) keyvalue.value;
      if (new_buf->valid) {
	fprintf(stderr, "DUPLICATE VALID BUFFERS!\n");
	exit(1);
      }

      pthread_mutex_lock(&new_buf->mutex); // hopeless
    }

    new_buf->buffer = malloc(init->buffer_size);
    new_buf->tag = init->tag;
    new_buf->prev_tag = init->prev_tag;
    new_buf->last_block = init->last_block;
    new_buf->buffer_size = init->buffer_size;
    new_buf->valid = true;

    if (!found) {
      // add to the hash table
      HTKeyValue oldkeyvalue;
      HTKeyValue newkeyvalue;
      
      newkeyvalue.value = (void *) new_buf;
      newkeyvalue.key = new_buf->tag;
		       
      int ret = InsertHashTable(ht, newkeyvalue, &oldkeyvalue);
      
      if(ret == 2) {
	// there was already a buffer by that name, so fix our next_tag and remove it
	struct buffer_info* repeat = (struct buffer_info*) oldkeyvalue.value;
	
	new_buf->next_tag = repeat->next_tag;
	
	if (repeat->waiting_queue != NULL) {
	  char* old_block;
	  while(!is_empty(repeat->waiting_queue)) {
	    dequeue(repeat->waiting_queue, (char**) &old_block);
	    handle_block(ht, old_block, arg);
	  }
	  free_queue(repeat->waiting_queue); // sync disaster
	  repeat->waiting_queue = NULL;
	}

	free(repeat);
      }
    } else {
      pthread_mutex_unlock(&new_buf->mutex);
      if (new_buf->waiting_queue != NULL) {
	char* old_block;
	while(!is_empty(new_buf->waiting_queue)) {
	  dequeue(new_buf->waiting_queue, (char**) &old_block);
	  handle_block(ht, old_block, arg);
	}
	free_queue(new_buf->waiting_queue); // sync disaster
	new_buf->waiting_queue = NULL;
      }
    }
  } else {
    struct data_block* buffer_data = (struct data_block*) block;

    uint32_t data_size = buffer_data->block_size - sizeof(struct data_block);

    // this must be data for an existing buffer    
    HTKeyValue buffer_kv;
    struct buffer_info* buffer;

    if (LookupHashTable(ht, buffer_data->tag, &buffer_kv) != 1) {
      // this buffer hasn't been initialized yet!

      struct queue* waiting_queue = allocate_queue();
      enqueue(waiting_queue, block, buffer_data->block_size);

      if (insert_dummy_buffer(ht, buffer_data->tag, &buffer, NO_TAG, waiting_queue)) {
	// successfully added dummy
	return;
      } else {
	if (!buffer->valid) {
	  // we didn't actually find the buffer, just a dummy
	  if (buffer->waiting_queue != NULL) {
	    enqueue(buffer->waiting_queue, block, buffer_data->block_size);
	    free_queue(waiting_queue);
	    return;
	  } else {
	    buffer->waiting_queue = waiting_queue;
	    return;
	  }
	} else {
	  // we found a real initialization block!
	  free_queue(waiting_queue);
	}
      }
    } else {

      // we found the buffer object, thank god
      buffer = (struct buffer_info*) buffer_kv.value;
      if (!buffer->valid) {
	if (buffer->waiting_queue == NULL) {
	  buffer->waiting_queue = allocate_queue();
	}
	enqueue(buffer->waiting_queue, block, buffer_data->block_size);
	return;
      }
    }
    
    memcpy(&buffer->buffer[buffer_data->seq_number], &block[sizeof(struct data_block)], data_size);

    pthread_mutex_lock(&buffer->mutex);
    
    buffer->n_written += data_size;
    uint32_t n_written = buffer->n_written;
    pthread_mutex_unlock(&buffer->mutex);
    if(n_written == buffer->buffer_size) {
      buffer->completed = true;
      handle_finished_buffer_prev(ht, buffer_kv, arg);
    }    
  }
}

void handle_finished_buffer_prev(HashTable ht, HTKeyValue buffer_kv, struct mp_reader_init* arg) {
  struct buffer_info* done = (struct buffer_info*) buffer_kv.value;

  if (done->prev_tag != NO_TAG) {
    // we have a dependancy
    HTKeyValue prev_buffer_kv;
    struct buffer_info* prev_buffer;
    if(LookupHashTable(ht, done->prev_tag, &prev_buffer_kv) == 0) {  
      // our dependancy doesn't exist! we'll have to make it and hope it comes in later

      // either inserts a new buffer or grabs the old one
      insert_dummy_buffer(ht, done->prev_tag, &prev_buffer, done->tag, NULL);
    } else
      prev_buffer = (struct buffer_info*) prev_buffer_kv.value;
    
    if (!prev_buffer->sent) {
      // we're being blocked by the prev, we have to wait for it to send us
      prev_buffer->next_tag = done->tag; //TODO sync
      return; // can't do anything else
    } else {
      // the dependancy is no longer useful, so we can free it
      RemoveFromHashTable(ht, done->prev_tag, &prev_buffer_kv);
      free(prev_buffer);
    }
  }

  // either we have no dependancies or they've been sent
  handle_finished_buffer_next(ht, buffer_kv, arg);
}

void handle_finished_buffer_next(HashTable ht, HTKeyValue buffer_kv, struct mp_reader_init* arg) {
  struct buffer_info* buffer = (struct buffer_info*) buffer_kv.value;

  arg->handler(buffer->buffer, buffer->buffer_size, arg->handler_arg);

  // TODO: sycn
  HTKeyValue next_buf_kv;
  if(!buffer->last_block && LookupHashTable(ht, buffer->next_tag, &next_buf_kv) == 1) {
    struct buffer_info* next_buf = (struct buffer_info*) next_buf_kv.value;
    if (next_buf->completed) {
      handle_finished_buffer_next(ht, next_buf_kv, arg); // recursively send buffers that depend on us
      goto done;
    } else
      goto zombie; // we have to wait for them to be done
  } else {
    // we can't free, someone's still waiting on us
    goto zombie;
  }

 done:
  //free
  RemoveFromHashTable(ht, buffer->tag, &buffer_kv);
 zombie:
  // we can't free since someone still might care that the packet was read
  buffer->sent = true;
}
