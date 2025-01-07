#include "funkcje_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MSG_TYPE_PERMISSION 1 // Typ wiadomości dla zezwolenia na wejście na statek
#define MSG_TYPE_RETURNED 10   // Typ wiadomości dla powrotu statku do portu
#define T2 10                 // Czas trwania rejsu w sekundach                  // Liczba rejsów
#define T1 60                 // Statek odpływa co minutę
#define R 1

int main() 
{
    clear_existing_message_queue(".", 'k');
    for (int rejs = 0; rejs < R; rejs++) 
    {
        printf("\n=== Rozpoczynam rejs %d ===\n", rejs + 1);

        // Inicjalizacja kolejki komunikatów
        int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);

        if (rejs == 0)
        {
            struct message signal_to_passengers;
            signal_to_passengers.type = MSG_TYPE_PERMISSION; // Typ wiadomości (zezwolenie)
            signal_to_passengers.content = 1;               // Treść wiadomości (np. kod "zezwalam")
            

            printf("KapitanStatku: Przygotowuję statek do wsiadania pasażerów...\n");
            sleep(3); // Symulacja przygotowań

            printf("KapitanStatku: Wysyłam sygnał do pasażerów: Można wchodzić na statek.\n");
            send_message_to_queue(message_queue_ID, &signal_to_passengers, 0);
        }

        // Odbiór sygnału zakończenia wsiadania
        struct message received_end_signal;
        printf("KapitanStatku: Czekam na sygnał o zakończeniu wsiadania pasażerów...\n");
        receive_message_from_queue(message_queue_ID, &received_end_signal, 2, 0);

        printf("\nKapitanStatku: Otrzymałem sygnał, że wszyscy pasażerowie są na statku. Przygotowuję się do rejsu.\n");

        // Odczekanie 10 sekund przed odpłynięciem (zamykanie wsiadania)
        printf("KapitanStatku: Wsiadanie zakończone. Odpływam za 10 sekund...\n");
        sleep(10);

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


        // pasazerowie zakonczyli opuszczanie statku. mozna ladowac nowych
        struct message passengers_disembarked_signal;
        printf("KapitanStatku: Czekam na sygnał, że wszyscy pasażerowie zeszli ze statku...\n");
        receive_message_from_queue(message_queue_ID, &passengers_disembarked_signal, 3, 0);
        printf("KapitanStatku: Otrzymano sygnał, że wszyscy pasażerowie zeszli ze statku.\n");


        // Wysłanie sygnału o rozpoczęciu kolejnego wsiadania
        if (rejs != R - 1)
        {
            struct message signal_to_passengers;
            signal_to_passengers.type = MSG_TYPE_PERMISSION; // Typ wiadomości (zezwolenie)
            signal_to_passengers.content = 1;               // Treść wiadomości (np. kod "zezwalam")
            printf("KapitanStatku: Wysyłam sygnał do pasażerów: Można wchodzić na statek.\n");
            send_message_to_queue(message_queue_ID, &signal_to_passengers, 0);

            // Oczekiwanie przed kolejnym rejem
            printf("KapitanStatku: Czekam [%d] sekund przed rozpoczęciem kolejnego rejsu...\n", T1);
            sleep(T1);
        }

        // Na zakończenie ostatniego rejsu usuń kolejkę komunikatów
        if (rejs == R - 1) 
        {
            delete_message_queue(message_queue_ID);
            printf("KapitanStatku: Usuwam kolejkę komunikatów i kończę operację.\n");
        }
    }

    return 0;
}