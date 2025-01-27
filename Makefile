CC = gcc
CFLAGS = -Wall -Wextra -ggdb -Wswitch-enum -Iinclude

all: server

server: server.c include/data_structures.h include/jutils.h include/webserver.h include/jacon.h include/http.h
	$(CC) $(CFLAGS) -o server server.c

clean:
	rm -f server