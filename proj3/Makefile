CC=gcc
CFLAGS= -g -Wall -std=gnu99 -pthread
DEPS = utils.h HashTable.h multiplex_standards.h multiplex_reader.h LinkedList.h multiplex_writer.h queue.h
OBJ = utils.o HashTable.o multiplex_reader.o LinkedList.o multiplex_writer.o queue.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: server client

server: $(OBJ) server.o
	gcc -o $@ $^ $(CFLAGS)

client: $(OBJ) client.o
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm *~ server client *.o
