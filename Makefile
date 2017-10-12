.PHONY: clean
CC=gcc
CFLAGS=-g -std=c89 -pedantic -Wall -Werror

tee : tee.o
	$(CC) tee.o -o tee $(CFLAGS)

tee.o : tee.c 
	$(CC) -c tee.c -o tee.o $(CFLAGS)

clean:
	rm *.o