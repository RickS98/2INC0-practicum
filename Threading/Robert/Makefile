#
#	Makefile for 2INC0 Interprocess Communication
#
#	(c) TUe 2010-2018, Joris Geurts
#

BINARIES = flip

CC = gcc
CFLAGS = -Wall -g -c -pthread
LDLIBS = -lrt -lm -lpthread

all:	$(BINARIES)

clean:
	rm -f *.o $(BINARIES)

flip: flip.o

flip.o: flip.c flip.h
