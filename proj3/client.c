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
#include "multiplex_writer.h"
#include "multiplex_standards.h"
#include "queue.h"

char* test_input = "client_testin.dat";
char* test_output = "client_testout.dat";

uint32_t udp_handshake(char* serv_address, uint32_t udp_port, int num_conn);
void test_writing(struct queue** queues, int num_queues);
void save_buffer(char* buffer, uint32_t buffer_len, void* arg);

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <SERVER ADDRESS> <SERVER PORT> <NIC ADDRESS 1> [NIC ADDRESS 2] ... [NIC ADDRESS n]\n", argv[0]);
    exit(1);
  }

  // delete the output file
  remove(test_output);

  char* serv_address = argv[1];
  uint32_t udp_port = atoi(argv[2]);
  int num_conn = argc - 3;

  uint32_t tcp_port = udp_handshake(serv_address, udp_port, num_conn);

  fprintf(stderr, "handshake successful, using tcp port %u\n", tcp_port);

  struct sockaddr_in servaddr;
  struct mp_writer_init* writer_init;
  struct mp_reader_init* reader_init;
 
  HashTable ht = AllocateHashTable(256);

  pthread_t threads[num_conn];
  pthread_t thread;
  int fd[num_conn];
  struct queue* queues[num_conn];

  for (int i = 0; i < num_conn; i++) {
    if (open_tcp(serv_address, tcp_port, &servaddr, &fd[i], argv[i+3]) == -1) {
      fprintf(stderr, "opening tcp failed on NIC address %s\n", argv[i+3]);
      exit(1);
    }
    
    //init writers and readers with fd
    queues[i] = allocate_queue();
    writer_init = (struct mp_writer_init*) malloc(sizeof(struct mp_writer_init));
    writer_init->client_fd = fd[i];
    writer_init->queue = queues[i];
    pthread_create(&threads[i], NULL, multiplex_writer_init, (void *)writer_init);

    reader_init = (struct mp_reader_init*) malloc(sizeof(struct mp_reader_init));
    reader_init->client_fd = fd[i];
    reader_init->ht = ht;
    reader_init->handler = save_buffer;
    reader_init->handler_arg = NULL;
    pthread_create(&thread, NULL, multiplex_reader_init, (void *) reader_init);
  }

  test_writing(queues, num_conn);

  //wait to exit so things can send
  sleep(60);
}

void save_buffer(char* buffer, uint32_t buffer_len, void* arg) {
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

void test_writing(struct queue** queues, int num_queues) {
  FILE* file = fopen (test_input, "rt");

  if (file == NULL) {
    fprintf(stderr, "problem opening file\n");
    return;
  }
  
  int buffer_size = 210000;
  char buffer[buffer_size];

  int read_size = fread(buffer, 1, buffer_size, file);
  multiplex_write_queues(queues, num_queues, buffer, read_size);

  /*
  int read_size;
  while( (read_size = fread(buffer, 1, buffer_size, file)) ) {
    multiplex_write_queues(queues, num_queues, buffer, read_size);
  }
  */
  
  

  fclose(file);  /* close the file prior to exiting the routine */
}

uint32_t udp_handshake(char* serv_address, uint32_t udp_port, int num_conn) {
  int udp_fd;
  struct sockaddr_in servaddr;
  
  open_udp(serv_address, udp_port, &servaddr, &udp_fd);

  int net_order_num_conn = htonl(num_conn);
  send_udp((char *)&net_order_num_conn, sizeof(num_conn), &servaddr, udp_fd);

  uint16_t port;

  int rec_size = recieve_udp((char*) &port, sizeof(port), &servaddr, udp_fd, 3);
  if (rec_size == -1) {
    fprintf(stderr, "problem recieving udp response from server during handshake, probably timeout\n");
    exit(1);
  }

  return ntohs(port);
}

