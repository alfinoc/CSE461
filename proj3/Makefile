CC=gcc
CFLAGS= -g -Wall -std=gnu99 -pthread
DEPS = utils.h HashTable.h multiplex_standards.h multiplex_reader.h LinkedList.h
OBJ = utils.o HashTable.o multiplex_reader.o server.o LinkedList.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

test_client:
	gcc -g -Wall -std=gnu99 -o client -pthread utils.c queue.c multiplex_writer.c

all: server test_client

clean:
	rm *~ server client *.o
