#ifndef MULTIPLEX_WRITER_HEADER
#define MUTLIPLEX_WRITER_HEADER

struct mp_writer_init {
  // The file descriptor for a TCP connection to the client
  int client_fd;
  // The shared hash table for storing buffers
  struct queue* queue;
};

/*
 * arg should be a (handler_init struct*)
 */
void *multiplex_writer_init(void* arg);

#endif
