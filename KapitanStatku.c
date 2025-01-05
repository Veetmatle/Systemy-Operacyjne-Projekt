#include "funkcje_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MSG_TYPE_PERMISSION 1 // Typ wiadomości dla zezwolenia na wejście na statek
#define T2 10 // Czas trwania rejsu w sekundach

int main() 
{
    // Inicjalizacja kolejki komunikatów
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);

    struct message signal_to_passengers;
    signal_to_passengers.type = MSG_TYPE_PERMISSION; // Typ wiadomości (zezwolenie)
    signal_to_passengers.content = 1; // Treść wiadomości (np. kod "zezwalam")

    printf("KapitanStatku: Przygotowuję statek do wsiadania pasażerów...\n");
    sleep(3); // Symulacja przygotowań

    printf("KapitanStatku: Wysyłam sygnał do pasażerów: Można wchodzić na statek.\n");
    send_message_to_queue(message_queue_ID, &signal_to_passengers, 0);

    // Odbiór sygnału zakończenia wsiadania
    struct message received_end_signal;
    printf("KapitanStatku: Czekam na sygnał o zakończeniu wsiadania pasażerów...\n");
    receive_message_from_queue(message_queue_ID, &received_end_signal, 2, 0);
    printf("\nKapitanStatku: Otrzymałem sygnał, że wszyscy pasażerowie są na statku. Rozpoczynam rejs.\n");

    // Symulacja rejsu
    printf("KapitanStatku: Statek odpływa...\n");
    for (int i = 0; i < T2; i++) 
    {
        printf("KapitanStatku: Statek w drodze... %d sekund\n", i + 1);
        sleep(1);
    }
    printf("\nKapitanStatku: Statek zakończył rejs.\n");

    // Wysłanie wiadomości o powrocie statku do portu
    struct message return_signal;
    return_signal.type = MSG_TYPE_RETURNED;
    return_signal.content = 1;
    send_message_to_queue(message_queue_ID, &return_signal, 0);
    printf("KapitanStatku: Wysłano sygnał do pasażerów: Statek wrócił do portu.\n");

    // Usuwanie kolejki komunikatów
    delete_message_queue(message_queue_ID);
    printf("KapitanStatku: Usuwam kolejkę komunikatów i kończę operację.\n");

    return 0;
}
