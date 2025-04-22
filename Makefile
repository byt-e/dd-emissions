CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LIBS = -lcurl -ljansson

all: ddfetch

ddfetch: main.c
	$(CC) $(CFLAGS) -o ddfetch main.c $(LIBS)

clean:
	rm -f ddfetch
	rm -f emissions.txt
