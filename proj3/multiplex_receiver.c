#include "mulitplex_handler.h"
#include "utils.h"

void listen(handler_init* params);

void *init(void *arg) {
  struct handler_init* params = (struct handler_init*) arg;
  
  listen(params);
}

void listen(handler_init* params) {
  void *block = malloc(BLOCK_SIZE);
  while(true) {
    
  }
}
