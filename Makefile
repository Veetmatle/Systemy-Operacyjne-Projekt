# Kompilator i flagi
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I.

# Pliki źródłowe i nagłówki
SRC_STATKU = KapitanStatku.c funkcje_kolejka_komunikatow.c funkcje_shared_memory.c zadanie_semafory.c
SRC_PORTU = KapitanPortu.c funkcje_kolejka_komunikatow.c funkcje_shared_memory.c zadanie_semafory.c
SRC_PASAZER = Pasazer.c funkcje_kolejka_komunikatow.c funkcje_shared_memory.c zadanie_semafory.c

HEADERS = funkcje_header.h

# Nazwy programów
BIN_STATKU = KapitanStatku
BIN_PORTU = KapitanPortu
BIN_PASAZER = Pasazer

# Domyślna reguła
all: $(BIN_STATKU) $(BIN_PORTU) $(BIN_PASAZER)

# Reguły dla KapitanStatku
$(BIN_STATKU): $(SRC_STATKU) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC_STATKU) -o $@

# Reguły dla KapitanPortu
$(BIN_PORTU): $(SRC_PORTU) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC_PORTU) -o $@

# Reguły dla Pasazer
$(BIN_PASAZER): $(SRC_PASAZER) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC_PASAZER) -o $@

# Reguła czyszczenia
clean:
	rm -f $(BIN_STATKU) $(BIN_PORTU) $(BIN_PASAZER)

# Phony targets
.PHONY: all clean
