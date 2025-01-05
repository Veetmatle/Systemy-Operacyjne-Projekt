#include "funkcje_header.h"
#include <time.h>
#include <sys/wait.h> // Dodanie biblioteki dla funkcji wait()

#define MAX_ON_BRIDGE 5    // Maksymalna liczba osób na kładce
#define MAX_ON_SHIP 10     // Maksymalna liczba osób na statku

int main() 
{
    // Klucze do semaforów
    key_t key_bridge = ftok(".", 'b'); 
    key_t key_ship = ftok(".", 's');   
    key_t key_msg_queue = ftok(".", 'k'); // Klucz do kolejki komunikatów

    if (key_bridge == -1 || key_ship == -1 || key_msg_queue == -1) 
    {
        perror("Błąd ftok");
        exit(1);
    }

    // Inicjalizacja semaforów
    int sem_bridge = initialize_semaphores(key_bridge, 1);
    int sem_ship = initialize_semaphores(key_ship, 1);

    // Ustawienie maksymalnych wartości semaforów
    semctl(sem_bridge, 0, SETVAL, MAX_ON_BRIDGE);
    semctl(sem_ship, 0, SETVAL, MAX_ON_SHIP);

    // Inicjalizacja kolejki komunikatów
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);

    // Flaga do pamięci współdzielonej (statek pełny)
    int shared_mem_id = initialize_shared_memory(".", 'f', sizeof(int), IPC_CREAT | 0666);
    int *ship_full_flag = (int *)attach_shared_memory(shared_mem_id, NULL, 0);
    *ship_full_flag = 0; // Początkowo statek nie jest pełny

    struct message received_signal;
    printf("Pasażerowie: Czekam na sygnał od KapitanaStatku...\n");
    receive_message_from_queue(message_queue_ID, &received_signal, MSG_TYPE_PERMISSION, 0);

    printf("\nPasażerowie: otrzymali sygnał i rozpoczynają wsiadanie...\n");

    // Symulacja wchodzenia pasażerów
    for (int i = 0; i < MAX_ON_SHIP + 10; i++) 
    {
        if (fork() == 0) // Tworzenie procesu dla każdego pasażera
        {
            printf("Pasażer [%d]: Próbuje wejść na kładkę...\n", getpid());
            if (*ship_full_flag == 1) 
            {
                printf("Pasażer [%d]: Statek jest pełny! Nie wchodzę na kładkę.\n", getpid());
                exit(0); // Proces kończy działanie
            }

            semaphore_wait(sem_bridge, 0, 0); // Wejście na kładkę
            printf("Pasażer [%d]: Jest na kładce.\n", getpid());
            sleep(2); // Symulacja przechodzenia przez kładkę

            printf("Pasażer [%d]: Próbuje wejść na statek...\n", getpid());

            // Sprawdzenie, czy są wolne miejsca na statku
            if (semctl(sem_ship, 0, GETVAL) > 0) 
            {
                semaphore_wait(sem_ship, 0, 0); // Wejście na statek
                sleep(1);
                semaphore_signal(sem_bridge, 0, 0); // Zwalnianie miejsca na kładce
                printf("Pasażer [%d]: Jest na statku.\n", getpid());

                // Aktualizacja flagi, jeśli statek jest pełny
                if (semctl(sem_ship, 0, GETVAL) == 0) 
                {
                    *ship_full_flag = 1; // Statek zapełniony
                }
            } 
            else 
            {
                printf("Pasażer [%d]: Wszystkie miejsca na statku zajęte, schodzę z kładki.\n", getpid());
                semaphore_signal(sem_bridge, 0, 0); // Zwalnienie miejsca na kładce
                // Wyzwolenie sygnału dla pozostałych osób na kładce
                while (semctl(sem_bridge, 0, GETVAL) < MAX_ON_BRIDGE) 
                {
                    semaphore_signal(sem_bridge, 0, 0); // Pozwolenie na zejście z kładki
                    printf("Pasażer [%d]: Zmuszony do opuszczenia kładki.\n", getpid());
                    sleep(1); // Symulacja schodzenia
                }
            }

            sleep(1); // Symulacja zajmowania miejsca lub opuszczania kładki

            // Koniec procesu pasażera
            printf("Pasażer [%d]: Zakończył operację.\n", getpid());
            exit(0);
        }
        sleep(1); // Odstęp między pasażerami
    }

    // Oczekiwanie na zakończenie wszystkich procesów pasażerów
    for (int i = 0; i < MAX_ON_SHIP + 10; i++) 
    {
        wait(NULL);
    }


    // Wysłanie sygnału do KapitanaStatku
    struct message end_signal;
    end_signal.type = 2; // Typ wiadomości - zakończenie wsiadania
    end_signal.content = 1;

    printf("\nPasażer: Wszyscy pasażerowie są na statku. Wysyłam sygnał do KapitanaStatku.\n");
    send_message_to_queue(message_queue_ID, &end_signal, 0);

    // Czekanie na sygnał powrotu statku
    struct message return_signal;
    printf("Pasażerowie: Czekam na sygnał od KapitanaStatku o powrocie statku do portu...\n");
    receive_message_from_queue(message_queue_ID, &return_signal, MSG_TYPE_RETURNED, 0);
    printf("\nPasażerowie: Statek wrócił do portu. Rozpoczynamy schodzenie ze statku.\n");

      // Symulacja schodzenia pasażerów
    for (int i = 0; i < MAX_ON_SHIP; i++) 
    {
        if (fork() == 0) 
        {
            semaphore_wait(sem_bridge, 0, 0); // Wejście na kładkę
            printf("Pasażer [%d]: Schodzi z kładki...\n", getpid());
            sleep(2); // Symulacja przechodzenia przez kładkę
            semaphore_signal(sem_bridge, 0, 0); // Zwalnianie miejsca na kładce
            exit(0);
        }
        sleep(1); // Odstęp między pasażerami
    }

    // Oczekiwanie na zakończenie procesów schodzenia
    for (int i = 0; i < MAX_ON_SHIP; i++) 
    {
        wait(NULL);
    }

    printf("\nPasażerowie: Wszyscy pasażerowie zeszli ze statku.\n");
    
    // Zwolnienie zasobów
    detach_shared_memory(ship_full_flag, shared_mem_id);
    delete_shared_memory(shared_mem_id);

    return 0;
}
