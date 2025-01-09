CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: server

server: server.c data_structures.h jutils.h webserver.h
	$(CC) $(CFLAGS) -o server server.c

clean:
	rm -f server