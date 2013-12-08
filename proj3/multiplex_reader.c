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
  int next_tag;
  bool last_block;
  pthread_mutex_t mutex;
};

void listen_for_block(struct handler_init* params);
void handle_block(HashTable ht, char* block);
void handle_finished_buffer_prev(HashTable ht, HTKeyValue buffer_kv);
void handle_finished_buffer_next(HashTable ht, HTKeyValue buffer_kv);
void send_buffer(void* buffer, uint32_t buffer_size);

void *multiplex_reader_init(void *arg) {
  struct handler_init* params = (struct handler_init*) arg;
  
  listen_for_block(params);
  return NULL;
}

void listen_for_block(struct handler_init* params) {
  char *block = malloc(BLOCK_SIZE);
  
  fprintf(stderr, "waiting for block on tcp %d\n", params->client_fd);
  while(true) {
    recieve_tcp(block, BLOCK_SIZE, params->client_fd, 0);
    handle_block(params->ht, block);
  }
}

void handle_block(HashTable ht, char* block) {
  int* head = (int*) block;
  if (*head == -1) {
    // initialize a new buffer
    struct init_block* init = (struct init_block*) block;
    HTKeyValuePtr keyvalue = NULL;
    int suc = LookupHashTable(ht, init->tag, keyvalue);
    
    struct buffer_info* new_buf;

    if (!suc) {
      new_buf = (struct buffer_info*) malloc(sizeof(struct buffer_info));
      memset(new_buf, '\0', sizeof(struct buffer_info));
      pthread_mutex_init(&new_buf->mutex, NULL);
    } else {
      new_buf = (struct buffer_info*) keyvalue->value;
      pthread_mutex_lock(&new_buf->mutex);
    }

    new_buf->buffer = malloc(init->buffer_size);
    new_buf->tag = init->tag;
    new_buf->prev_tag = init->prev_tag;
    new_buf->last_block = init->last_block;
    new_buf->buffer_size = init->buffer_size;
    
    if (!suc) {
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
	free(repeat);
      }
    } else {
      pthread_mutex_unlock(&new_buf->mutex);
    }
  } else {
    struct data_block* buffer_data = (struct data_block*) head;

    // this must be data for an existing buffer    
    HTKeyValue buffer_kv;

    while(LookupHashTable(ht, buffer_data->tag, &buffer_kv) != 1) {
      fprintf(stderr, "buffer not found in the hash table! spinning..\n");
    }
    
    // we found the buffer object, thank god
    struct buffer_info* buffer = (struct buffer_info*) buffer_kv.value;
   
    uint32_t copy_size = BLOCK_SIZE;
    uint32_t start_index = buffer_data->seq_number * BLOCK_SIZE;
    if (start_index + BLOCK_SIZE >= buffer->buffer_size) {
      // this must be the last packet, so it might be shorter..
      copy_size = buffer->buffer_size - start_index;
    }

    memcpy(&buffer->buffer[start_index], &block[sizeof(struct data_block)], copy_size);

    pthread_mutex_lock(&buffer->mutex);
    uint32_t n_written = ++buffer->n_written;
    pthread_mutex_unlock(&buffer->mutex);
    
    if(n_written * BLOCK_SIZE >= buffer->buffer_size) {
      buffer->completed = true;
      handle_finished_buffer_prev(ht, buffer_kv);
    }
  }
}

void handle_finished_buffer_prev(HashTable ht, HTKeyValue buffer_kv) {
  struct buffer_info* done = (struct buffer_info*) buffer_kv.value;
  fprintf(stderr, "there's a completed buffer of size %u. life is good\n", done->buffer_size);

  if (done->prev_tag != 0) {
    // we have a dependancy
    HTKeyValue prev_buffer_kv;
    struct buffer_info* prev_buffer;
    if(LookupHashTable(ht, done->prev_tag, &prev_buffer_kv) == 0) {  
      // our dependancy doesn't exist! we'll have to make it and hope it comes in later
      prev_buffer = calloc(1, sizeof(struct buffer_info));
      pthread_mutex_init(&prev_buffer->mutex, NULL);
      
      prev_buffer_kv.value = (void *)prev_buffer;
      prev_buffer_kv.key = done->prev_tag;

      HTKeyValue dummy;
      InsertHashTable(ht, prev_buffer_kv, &dummy); //TODO: sync
    }
    
    // we found (or made) the dependancy
    prev_buffer = (struct buffer_info*) prev_buffer_kv.value;
    if (!prev_buffer->sent) {
      // we're being blocked by the prev, we have to wait for it to send us
      prev_buffer->next_tag = done->tag; //TODO sync
      return;
    } else {
      // the dependancy is no longer useful, so we can free it
      RemoveFromHashTable(ht, done->prev_tag, &prev_buffer_kv);
      free(prev_buffer);
    }
  }

  // either we have no dependancies or they've been sent
  handle_finished_buffer_next(ht, buffer_kv);
}

void handle_finished_buffer_next(HashTable ht, HTKeyValue buffer_kv) {
  struct buffer_info* buffer = (struct buffer_info*) buffer_kv.value;
  
  send_buffer(buffer->buffer, buffer->buffer_size);

  // TODO: sycn
  HTKeyValue next_buf_kv;
  if(!buffer->last_block && LookupHashTable(ht, buffer->next_tag, &next_buf_kv) == 1) {
    struct buffer_info* next_buf = (struct buffer_info*) next_buf_kv.value;
    if (next_buf->completed) {
      handle_finished_buffer_next(ht, next_buf_kv); // recursively send buffers that depend on us
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
  free(buffer);
  printf("we freed a buffer!\n");
 zombie:
  // we can't free since someone still might care that the packet was read
  buffer->sent = true;
}

void send_buffer(void* buffer, uint32_t buffer_size) {
  printf("WE'RE SENDING A BUFFER OF SIZE %u! WOOOOOO\n", buffer_size);
  free(buffer);
}
