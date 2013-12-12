#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include "utils.h"
#include "multiplex_reader.h"
#include "queue.h"
#include "multiplex_writer.h"

#define MAX_CONN 5

char* test_output = "testout.dat";

struct client_info {
  int num_conn;
  int sock_fd;
  int sock_port;
};

typedef struct sockaddr_in* sockaddr_t;

int bind_random_port(struct sockaddr_in*, int type);
void* serve_client(void* arg);
void send_buffer(char* buffer, uint32_t buffer_len, void* arg);
void bounce_buffer(char* buffer, uint32_t buffer_len, void* bb_arg);

struct bounce_buf_arg {
  struct queue** queues;
  int n_queues;
};

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s PORT\n", argv[0]);
    exit(1);
  }

  // seed randomness
  srand(time(NULL));

  //delete test file
  remove(test_output);

  int first_port = atoi(argv[1]);

  // set up the perminant udp server
  struct sockaddr_in main_servaddr;
  int main_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  bzero(&main_servaddr, sizeof(main_servaddr));
  main_servaddr.sin_family = AF_INET;
  main_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  main_servaddr.sin_port = htons(first_port);
  bind(main_sockfd, (struct sockaddr *)&main_servaddr, sizeof(main_servaddr));

  struct sockaddr_in client_addr;
  int rec_size;
  struct client_info* arg;
  pthread_t pthread;
  struct sockaddr_in tcp_servaddr;
    
  while (1) {
    printf("waiting for udp handshake...\n");
    
    // prepare arg for new client thread
    arg = (struct client_info*) malloc(sizeof(struct client_info));
    if (arg == NULL) {
      fprintf(stderr, "error with malloc, errno %d\n", errno);
      exit(errno);
    }

    rec_size = recieve_udp((void*) &(arg->num_conn), sizeof(int),
                           (sockaddr_t) &client_addr, main_sockfd, 0);
    arg->num_conn = ntohl(arg->num_conn);


    if (rec_size == 0)
      continue;
    
    // debug stuff, makes netcat easier
    // rec_size = 4;
    // arg->num_conn = 2;

    // validate
    if (rec_size != sizeof(int) || arg->num_conn > MAX_CONN
       || arg->num_conn < 0) {
      perror("malformed initialization packet\n");
      continue;
    }

    arg->sock_fd = bind_random_port(&tcp_servaddr, SOCK_STREAM);
    arg->sock_port = ntohs(tcp_servaddr.sin_port);
    
    send_udp((char *)&tcp_servaddr.sin_port, 2,
	     &client_addr, main_sockfd);

    pthread_create(&pthread, NULL, serve_client, (void*) arg);
  }
}

void* serve_client(void* arg) {
  struct client_info* init = (struct client_info*) arg;
  //int* tcp_fds = (int*) malloc(sizeof(int) * init->num_conn);
  printf("requested number of connections: %d\n", init->num_conn);

  HashTable ht = AllocateHashTable(256);
  struct queue** queues = (struct queue**) malloc(init->num_conn * sizeof(struct queue));

  int client_fd;

  struct mp_reader_init* reader_init;
  struct mp_writer_init* writer_init;

  pthread_t thread;
  struct sockaddr_in tcp_clientaddr;
  socklen_t clientaddr_len = sizeof(tcp_clientaddr);

  struct bounce_buf_arg* bounce_buffer_arg = (struct bounce_buf_arg*) 
    malloc(sizeof(struct bounce_buf_arg));
  bounce_buffer_arg->queues = queues;
  bounce_buffer_arg->n_queues = init->num_conn;

  listen(init->sock_fd, init->sock_port);

  // set up tcp connections, store each in tcp_conns
  for (int i = 0; i < init->num_conn; i++) {
    client_fd = accept(init->sock_fd, (struct sockaddr*) &tcp_clientaddr, &clientaddr_len);
    printf("Accepted tcp: %d\n", client_fd);
    
    //init writers and readers with fd                                                                   
    queues[i] = allocate_queue();
    writer_init = (struct mp_writer_init*) malloc(sizeof(struct mp_writer_init));
    writer_init->client_fd = client_fd;
    writer_init->queue = queues[i];
    pthread_create(&thread, NULL, multiplex_writer_init, (void *)writer_init);

    reader_init = (struct mp_reader_init*) malloc(sizeof(struct mp_reader_init));
    reader_init->client_fd = client_fd;
    reader_init->ht = ht;
    reader_init->handler = bounce_buffer;
    reader_init->handler_arg = bounce_buffer_arg;
    pthread_create(&thread, NULL, multiplex_reader_init, (void *) reader_init);

  }

  return NULL;
}

void bounce_buffer(char* buffer, uint32_t buffer_len, void* bb_arg) {
  struct bounce_buf_arg* arg = (struct bounce_buf_arg*) bb_arg;
  struct queue** queues = arg->queues;
  int n_queues = arg->n_queues;

  printf("BOUNCING BUFFER OF SIZE %u BACK!\n", buffer_len);

  int next_tag = -1;

  //send the buffer back!
  multiplex_write_queues(queues, n_queues, buffer, buffer_len, &next_tag);

  free(buffer);
}

void send_buffer(char* buffer, uint32_t buffer_len, void* arg) {
  FILE* file = fopen(test_output, "a");
  if (file == NULL) {
    fprintf(stderr, "error opening output file\n");
    free(buffer);
    return;
  }

  printf("writing %u bytes to %s...\n", buffer_len, test_output);
  fwrite(buffer, 1, buffer_len, file);
  fclose(file);
  free(buffer);
}

int bind_random_port(sockaddr_t servaddr, int type) {
  uint16_t port;
  uint16_t port_range = 65524 - 256;
  int sockfd;
  int ret;

  sockfd = socket(AF_INET, type, 0);
  bzero(servaddr, sizeof(struct sockaddr_in));
  servaddr->sin_family = AF_INET;
  servaddr->sin_addr.s_addr=htons(INADDR_ANY);

  do {
    port = (rand() % port_range) + 256;
    servaddr->sin_port = htons(port);
    ret = bind(sockfd, (struct sockaddr *) servaddr, sizeof(struct sockaddr_in));
  } while (ret == -1);

  return sockfd;
}
