gcc -pthread -o KapitanStatku KapitanStatku.c funkcje_kolejka_komunikatow.c funkcje_shared_memory.c zadanie_semafory.c funkcje_header.h
gcc -pthread -o Pasazer Pasazer.c funkcje_kolejka_komunikatow.c funkcje_shared_memory.c zadanie_semafory.c funkcje_header.h
gcc -pthread -o KapitanPortu KapitanPortu.c funkcje_kolejka_komunikatow.c funkcje_shared_memory.c zadanie_semafory.c funkcje_header.h
