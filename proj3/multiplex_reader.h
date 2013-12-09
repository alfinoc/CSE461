#ifndef MULTIPLEX_READER_HEADER
#define MUTLIPLEX_READER_HEADER

#include "HashTable.h"
#include <sys/types.h>

typedef void (*buffer_handler)(char* buffer, uint32_t buffer_len, void* arg);

struct mt_reader_init {
  // The file descriptor for a TCP connection to the client
  int client_fd;
  // The shared hash table for storing buffers
  HashTable ht;
  // Function to pass completed buffers to
  // Buffers will only be passed in order
  buffer_handler handler;
  // Extra argument for the handler function
  void* listener_arg;
};

/*
 * arg should be a (handler_init struct*)
 */
void *multiplex_reader_init(void* arg);

#endif
