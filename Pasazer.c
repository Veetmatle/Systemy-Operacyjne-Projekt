#include "funkcje_header.h"
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main() 
{
    clear_existing_shared_memory(".", 'p');
    clear_existing_semaphores(".", 'b');
    clear_existing_semaphores(".", 's');
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);

    // Klucze do semaforów i pamięci współdzielonej
    key_t key_bridge = ftok(".", 'b'); 
    key_t key_ship = ftok(".", 's');   
    key_t key_msg_queue = ftok(".", 'k'); 
    key_t key_shared_pid = ftok(".", 'p'); 

    if (key_bridge == -1 || key_ship == -1 || key_msg_queue == -1 || key_shared_pid == -1) 
    {
        perror("Błąd ftok");
        exit(1);
    }

    // Inicjalizacja semaforów
    int sem_bridge = initialize_semaphores(key_bridge, 1);
    int sem_ship = initialize_semaphores(key_ship, 1);

    // Pamięć współdzielona do przechowywania PID-ów
    int shared_pid_mem_id = initialize_shared_memory(".", 'p', MAX_ON_SHIP * sizeof(pid_t), IPC_CREAT | 0666);
    pid_t *pids_on_ship = (pid_t *)attach_shared_memory(shared_pid_mem_id, NULL, 0);


    for (int rejs = 0; rejs < R; rejs++) 
    {
        // Flaga do pamięci współdzielonej (statek pełny)
        int shared_mem_id = initialize_shared_memory(".", 'f', sizeof(int), IPC_CREAT | 0666);
        int *ship_full_flag = (int *)attach_shared_memory(shared_mem_id, NULL, 0);
        *ship_full_flag = 0;

        // Ustawienie maksymalnych wartości semaforów
        semctl(sem_bridge, 0, SETVAL, MAX_ON_BRIDGE);
        semctl(sem_ship, 0, SETVAL, MAX_ON_SHIP);

        // Zerowanie pamięci współdzielonej dla PID-ów
        for (int i = 0; i < MAX_ON_SHIP; i++) 
        {
            pids_on_ship[i] = 0;
        }

        struct message received_signal;
        printf(LIGHTBLUE "Pasażerowie: Czekam na sygnał od KapitanaStatku...\n");
        receive_message_from_queue(message_queue_ID, &received_signal, MSG_TYPE_PERMISSION, 0);

        printf(LIGHTBLUE "Pasażerowie: otrzymali sygnał i rozpoczynają wsiadanie...\n");

        // Symulacja wchodzenia pasażerów
        for (int i = 0; i < MAX_ON_SHIP + 10; i++) 
        {
            pid_t pid = fork(); // Tworzenie procesu dla każdego pasażera
            if (pid == -1)
            {
                perror("Fork failed");
                return -1;
            }

            if (pid == 0)
            {
                printf(LIGHTBLUE "Pasażer [%d]: Próbuje wejść na kładkę...\n", getpid());
                if (*ship_full_flag == 1) 
                {
                    printf(LIGHTBLUE "Pasażer [%d]: Statek jest pełny! Nie wchodzę na kładkę.\n", getpid());
                    exit(0);
                }

                semaphore_wait(sem_bridge, 0, 0);
                printf(LIGHTBLUE "Pasażer [%d]: Jest na kładce.\n", getpid());
                usleep(3000);

                printf(LIGHTBLUE "Pasażer [%d]: Próbuje wejść na statek...\n", getpid());

                if (semctl(sem_ship, 0, GETVAL) > 0) 
                {
                    semaphore_wait(sem_ship, 0, 0); 
                    semaphore_signal(sem_bridge, 0, 0);
                    printf(LIGHTBLUE "Pasażer [%d]: Jest na statku.\n", getpid());

                    if (semctl(sem_ship, 0, GETVAL) == 0) 
                    {
                        *ship_full_flag = 1;
                    }

                    // Zapisanie PID pasażera do pamięci współdzielonej
                    for (int j = 0; j < MAX_ON_SHIP; j++) 
                    {
                        if (pids_on_ship[j] == 0) 
                        {
                            pids_on_ship[j] = getpid();
                            break;
                        }
                    }

                    pause(); // Oczekiwanie na sygnał od kapitana
                } 
                else 
                {
                    printf(LIGHTBLUE "Pasażer [%d]: Wszystkie miejsca na statku zajęte, schodzę z kładki.\n", getpid());
                    semaphore_signal(sem_bridge, 0, 0);
                    exit(0);
                }
            }
            usleep(1000);
        }

        // Oczekiwanie na zakończenie procesów, które nie weszły na statek
        for (int i = 0; i < MAX_ON_SHIP + 10 - MAX_ON_SHIP; i++) 
        {
            wait(NULL);
        }

        // Czekanie na sygnał powrotu statku
        struct message return_signal;
        printf(LIGHTBLUE "Pasażerowie: Czekam na sygnał od KapitanaStatku o powrocie do portu...\n");
        receive_message_from_queue(message_queue_ID, &return_signal, MSG_TYPE_RETURNED, 0);

        printf(LIGHTBLUE "\nPasażerowie: Rozpoczynam schodzenie ze statku.\n");

        // Schodzenie pasażerów
        for (int i = 0; i < MAX_ON_SHIP; i++) 
        {
            if (pids_on_ship[i] > 0) 
            {
                semaphore_wait(sem_bridge, 0, 0);
                printf(LIGHTBLUE "Pasażer [%d]: Schodzę ze statku...\n", pids_on_ship[i]);
                usleep(1000);
                semaphore_signal(sem_bridge, 0, 0);
                kill(pids_on_ship[i], SIGUSR1); // Sygnał dla procesu pasażera

                // zerujemy w pamieci dla kapitana: pasazer nie jest juz na statku
                pids_on_ship[i] = 0; 
            }
        }

        // Oczekiwanie na zakończenie procesów pasażerów
        for (int i = 0; i < MAX_ON_SHIP; i++) 
        {
            wait(NULL);
        }

        if(return_signal.content == 999)
        {
            detach_shared_memory(ship_full_flag, shared_mem_id); 
            delete_shared_memory(shared_mem_id);
            break;
        }
        detach_shared_memory(ship_full_flag, shared_mem_id);
        delete_shared_memory(shared_mem_id);
    }

    printf(RESET "\n=== Wszystkie rejsy zakończone ===\n");

    // Usunięcie semaforów
    destroy_semaphores(sem_bridge);
    destroy_semaphores(sem_ship);

    // Zwolnienie pamięci współdzielonej i odłączenie segmentów
    detach_shared_memory(pids_on_ship, shared_pid_mem_id);
    delete_shared_memory(shared_pid_mem_id);

    return 0;
}