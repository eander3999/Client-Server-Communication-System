
CC = gcc

CFLAGS = -g -Wall

TARGETS = clean tcpclient tcpserver

.PHONY: clean all

default: tcpclient tcpserver

all: $(TARGETS)

tcpclient: tcpclient.c
	$(CC) $(CFLAGS) $^ -o $@

async-tcpserver: tcpserver.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) tcpclient tcpserver
