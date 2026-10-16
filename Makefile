all: client server

CC=gcc
CFLAGS=-I. -lm -g

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

client: client.o
	$(CC) -o $@ $^ $(CFLAGS)

server: server.o game.o save.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f client
	rm -f server