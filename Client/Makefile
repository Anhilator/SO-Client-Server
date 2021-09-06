CC = gcc
CCOPTS=--std=gnu99 -Wall -ggdb3
AR=ar

OBJS=linked_list.o\

HEADERS=linked_list.h\
    client.h\
	common.h\


BINS=client\

.phony: clean all

all:	client

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@ $<
	
client: client.c $(OBJS)
	$(CC) $(CCOPTS) -o $@ $^



clean:
	rm -rf *.o *~ $(LIBS) $(BINS)
