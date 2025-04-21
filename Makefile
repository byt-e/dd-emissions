CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99
LIBS=-lcurl -ljansson

all: ddfetch

ddfetch: main.c
	$(CC) $(CFLAGS) -o ddfetch main.c $(LIBS)

clean:
	rm -f ddfetch
	rm -f emissions.txt
