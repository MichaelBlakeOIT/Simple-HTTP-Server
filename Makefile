# Makefile for http-server
#
# Author: Michael Blake
# Date:   Dec 1, 2017
#
COPTS = -g -O0 -Wall -std=c99 -pthread

OBJS =

all: main

clean:
	rm -f server
	rm -f *.o

.c.o:
	gcc $(COPTS) -c $? -o $@

main: main.c $(OBJS)
	gcc $(COPTS) main.c -o server $(OBJS)

