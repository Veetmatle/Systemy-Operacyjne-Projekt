#include "funkcje_header.h"
#include <time.h>

#define MAX_PASSENGERS 10 // Maksymalna liczba pasażerów na statku
#define MAX_BRIDGE 5      // Maksymalna liczba osób na mostku
#define T1 5              // Czas do odpłynięcia
#define T2 10             // Czas rejsu

int main() 
{
    // Inicjalizacja pamięci współdzielonej
    int shm_id = initialize_shared_memory(".", 'S', (MAX_PASSENGERS + 2) * sizeof(int), IPC_CREAT | 0666);
    int *shared_memory = (int *)attach_shared_memory(shm_id, NULL, 0);

    // Inicjalizacja semaforów
    key_t sem_key = ftok(".", 'M');
    int sem_id = initialize_semaphores(sem_key, 2);
    
    semctl(sem_id, 0, SETVAL, 1); // Semafor dla mostku (muteks)
    semctl(sem_id, 1, SETVAL, 1); // Semafor dla statku (muteks)

    // Inicjalizacja kolejek komunikatów
    int msg_queue_id = initialize_message_queue(".", 'Q', IPC_CREAT | 0666);

    struct message signal_msg;

    shared_memory[0] = 0; // Liczba osób na mostku
    shared_memory[1] = 0; // Liczba osób na statku

    printf("Kapitan Statku: Gotowy do pracy.\n");

    while (1) 
    {
        // Odbieranie sygnałów od Kapitana Portu
        receive_message_from_queue(msg_queue_id, &signal_msg, 1, 0);
        
        if (signal_msg.content == 1) {
            // Sygnał 1: Odpłynięcie przed czasem
            semaphore_wait(sem_id, 0, 0); // Zablokuj mostek

            while (shared_memory[0] > 0) 
            {
                if (shared_memory[1] < MAX_PASSENGERS) 
                {
                    printf("Pasażer wchodzi na statek.\n");
                    shared_memory[0]--;
                    shared_memory[1]++;
                    sleep(1);
                } else {
                    printf("Pasażer opuszcza mostek, brak miejsca na statku.\n");
                    shared_memory[0]--;
                    sleep(1);
                }
            }
            
            semaphore_signal(sem_id, 0, 0); // Odblokuj mostek
            printf("Kapitan Statku: Statek odpływa przed czasem.\n");
            sleep(T2); // Symulacja rejsu
            printf("Kapitan Statku: Rejs zakończony.\n");

            semaphore_wait(sem_id, 1, 0); // Zablokuj statek

            while (shared_memory[1] > 0) 
            {
                printf("Pasażer opuszcza statek.\n");
                shared_memory[1]--;
                sleep(1);
            }

            semaphore_signal(sem_id, 1, 0); // Odblokuj statek
        }
    }

    detach_shared_memory(shared_memory, shm_id);
    delete_shared_memory(shm_id);
    destroy_semaphores(sem_id);
    delete_message_queue(msg_queue_id);

    return 0;
}