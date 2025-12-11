CC = gcc
CFLAGS = -std=c17 -O2 -Wall -Wextra -Wpedantic

.PHONY: all clean run
all: Implementierung
Implementierung: Implementierung.c
	$(CC) $(CFLAGS) -o $@ $^
run: Implementierung
	./Implementierung	
clean:
	rm -f Implementierung
