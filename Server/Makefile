CC = gcc
CCOPTS=--std=gnu99 -Wall
AR=ar

OBJS=linked_list.o\

HEADERS=linked_list.h\
    private_chat_struct.h\
    server.h\
    common.h\

BINS=server\

.phony: clean all

all:	server 

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@ $<
	
server: server.c $(OBJS)
	$(CC) $(CCOPTS) -o $@ $^



clean:
	rm -f server
