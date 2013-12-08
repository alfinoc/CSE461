#include "mulitplex_handler.h"
#include "utils.h"
#include <stdbool.h>

struct buffer_info {
  void* buffer;
  uint32_t buf_size;
  int tag;
  int prev_tag;
  int terminal;
}

void listen(handler_init* params);

void *init(void *arg) {
  struct handler_init* params = (struct handler_init*) arg;
  
  listen(params);
}

void listen(handler_init* params) {
  void *block = malloc(BLOCK_SIZE);
  while(true) {
    recieve_tcp((char *)buf, BLOCK_SIZE, params->client_fd, 0);
    handle_block(params->ht, block);
  }
}

void handle_block(HashTable ht, void* block) {
  int* head = (int*)block;
  if (*head == -1) {
    // initialize a new buffer
    
  }
}
