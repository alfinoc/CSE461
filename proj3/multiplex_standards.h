#ifndef MULTIPLEX_STANDARDS
#define MULTIPLEX_STANDARDS

#include <stdbool.h>

#define BLOCK_SIZE 20
#define NO_TAG -1

struct data_block {
  // the size of this block, including the header
  uint32_t block_size;
  uint32_t seq_number;
  int tag;
};

struct init_block {
  // number of bytes in the message, in this case sizeof(struct init_block)
  uint32_t block_size;
  int flag; // should always be -1 in a init block
  uint32_t buffer_size; // size of buffer in bytes
  int tag; // unique id, cannot be NO_TAG
  int prev_tag; // tag of the block that must be sent before this one, NO_TAG if none
  bool last_block; // 1 if no block will depend on this one, otherwise 0
};

#endif
