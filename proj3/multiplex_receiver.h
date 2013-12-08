#ifndef MULTIPLEX_HANDLER_HEADER
#define MUTLIPLEX_HANDLER_HEADER

#define BLOCK_SIZE 1024

struct handler_init {
  // The file descriptor for a TCP connection to the client
  int client_fd;
  // The shared hash table for storing buffers
  HashTable ht;
};

/*
 * arg should be a (handler_init struct*)
 */
void *init(void* arg);

#endif
