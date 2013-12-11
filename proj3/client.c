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

char* test_input = "testin.dat";
char* test_output = "testout.dat";

uint32_t udp_handshake(char* serv_address, uint32_t udp_port, int num_conn);

void test_writing(struct queue** queues, int num_queues);

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <SERVER ADDRESS> <SERVER PORT> <NIC ADDRESS 1> [NIC ADDRESS 2] ... [NIC ADDRESS n]\n", argv[0]);
    exit(1);
  }

  char* serv_address = argv[1];
  uint32_t udp_port = atoi(argv[2]);
  int num_conn = argc - 3;

  uint32_t tcp_port = udp_handshake(serv_address, udp_port, num_conn);

  fprintf(stderr, "handshake successful, using tcp port %u\n", tcp_port);

  struct sockaddr_in servaddr;
  struct mp_writer_init* writer_init;
 
  pthread_t threads[num_conn];
  int fd[num_conn];
  struct queue* queues[num_conn];

  for (int i = 3; i < argc; i++) {
    if (open_tcp(serv_address, tcp_port, &servaddr, &fd[i-3], argv[i]) == -1) {
      fprintf(stderr, "opening tcp failed on NIC address %s\n", argv[i]);
      exit(1);
    }
    
    //init writers and readers with fd
    writer_init = (struct mp_writer_init*) malloc(sizeof(struct mp_writer_init));
    writer_init->client_fd = fd[i-3];
    queues[i] = allocate_queue();
    writer_init->queue = queues[i];

    pthread_create(&threads[i], NULL, multiplex_writer_init, (void *)writer_init);
  }

  test_writing(queues, num_conn);

  //wait to exit so things can send
  sleep(5);
}

void test_writing(struct queue** queues, int num_queues) {
  int buffer_size = 65536;
  char buffer[buffer_size];

  FILE* file = fopen (test_input, "rt");

  if (file == NULL) {
    fprintf(stderr, "problem opening file\n");
    return;
  }
  
  int read_size;
  while( (read_size = fread(buffer, 1, buffer_size, file)) ) {
    fprintf(stderr, "writing buffer chunk of size %d\n", read_size);
    multiplex_write_queues(queues, num_queues, buffer, read_size);
  }

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

