CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LIBS = -lcurl -ljansson

all: ddfetch

ddfetch: main.c
	$(CC) $(CFLAGS) -o ddfetch main.c $(LIBS)

report:
	@echo "📊 Generating Emissions Report"
	@awk -f awk/total.awk emissions.txt
	@awk -f awk/max.awk emissions.txt
	@awk -f awk/average.awk emissions.txt

clean:
	rm -f ddfetch
	rm -f emissions.txt
