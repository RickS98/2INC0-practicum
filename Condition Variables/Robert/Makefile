#
#	Makefile for 2INC0 Interprocess Communication
#
#	(c) TUe 2010-2018, Joris Geurts
#

BINARIES = prodcons

CC = gcc
CFLAGS = -Wall -g -c -pthread
LDLIBS = -lrt -lm -lpthread

all:	$(BINARIES)

clean:
	rm -f *.o $(BINARIES)

prodcons: prodcons.o

prodcons.o: prodcons.c prodcons.h
