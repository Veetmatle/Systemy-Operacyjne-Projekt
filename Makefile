# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -O2

# Common files
HEADERS = funkcje_header.h
COMMON_SRC = funkcje_kolejka_komunikatow.c funkcje_shared_memory.c zadanie_semaforow.c

# Targets
TARGETS = KapitanStatku Pasażer KapitanPortu

# Rules
all: $(TARGETS)

KapitanStatku: KapitanStatku.c $(COMMON_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ KapitanStatku.c $(COMMON_SRC)

Pasażer: Pasażer.c $(COMMON_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ Pasażer.c $(COMMON_SRC)

KapitanPortu: KapitanPortu.c $(COMMON_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ KapitanPortu.c $(COMMON_SRC)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
