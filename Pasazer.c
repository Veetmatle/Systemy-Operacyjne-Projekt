#include "funkcje_header.h"

volatile bool reaper_running = true;

void* reaper_thread(void* arg) 
{
    while (reaper_running) 
    {
        while (waitpid(-1, NULL, WNOHANG) > 0) {}
        usleep(1000); 
    }
    return NULL;
}

int main() 
{
    clear_existing_shared_memory(".", 'e');
    clear_existing_shared_memory(".", 'p');
    clear_existing_semaphores(".", 'b');
    clear_existing_semaphores(".", 's');
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);

    key_t key_bridge = ftok(".", 'b'); 
    key_t key_ship = ftok(".", 's');   
    key_t key_msg_queue = ftok(".", 'k'); 
    key_t key_shared_pid = ftok(".", 'p'); 
    key_t key_synchro_proces = ftok(".", 'u');

    if (key_bridge == -1 || key_ship == -1 || key_msg_queue == -1 || key_shared_pid == -1 || key_synchro_proces == -1) 
    {
        perror("Błąd ftok");
        exit(1);
    }

    // Inicjalizacja semaforów
    int sem_bridge = initialize_semaphores(key_bridge, 1);
    int sem_ship = initialize_semaphores(key_ship, 1);
    int sem_synchro = initialize_semaphores(key_synchro_proces, 1);
    semctl(sem_synchro, 0, SETVAL, 0);

    // Pamięć współdzielona do przechowywania PID-ów
    int shared_pid_mem_id = initialize_shared_memory(".", 'p', MAX_ON_SHIP * sizeof(pid_t), IPC_CREAT | 0666);
    pid_t *pids_on_ship = (pid_t *)attach_shared_memory(shared_pid_mem_id, NULL, 0);

    // Inicjalizacja pamięci współdzielonej dla flagi koniec_wchodzenia
    int shared_end_boarding_id = initialize_shared_memory(".", 'e', sizeof(int), IPC_CREAT | 0666);
    int *koniec_wchodzenia = (int *)attach_shared_memory(shared_end_boarding_id, NULL, 0);

    // Dostęp do pamięci współdzielonej dla licznika
    int shared_counter_id = initialize_shared_memory(".", 'c', sizeof(int), IPC_CREAT | 0666);
    int *shared_counter = (int *)attach_shared_memory(shared_counter_id, NULL, 0);
    *shared_counter = 0;

    // pamiec wspoldzelona - flaga czy statek pełny
    int shared_mem_id = initialize_shared_memory(".", 'f', sizeof(int), IPC_CREAT | 0666);
    int *ship_full_flag = (int *)attach_shared_memory(shared_mem_id, NULL, 0);

    // Utworzenie watku zbierajacego procesy
    pthread_t reaper_tid;
    if (pthread_create(&reaper_tid, NULL, reaper_thread, NULL) != 0) 
    {
        perror("Błąd tworzenia wątku");
        exit(1);
    }

    for (int rejs = 0; rejs < R; rejs++) 
    {
        *ship_full_flag = 0;
        *shared_counter = 0;

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
            pid_t pid = fork(); 
            if (pid == -1)
            {
                perror("Fork failed");
                system("./clear_ipcs.sh"); 
                return -1;
            }

            if (pid == 0)
            { 
                printf(LIGHTBLUE "Pasażer [%d]: Próbuje wejść na kładkę...\n", getpid());
                 if (*koniec_wchodzenia == 1) 
                {
                    printf(LIGHTBLUE "Pasażer [%d]: Wchodzenie zostało zakończone, nie wchodzę.\n", getpid());
                    exit(0);
                }

                if (*ship_full_flag == 1) 
                {
                    printf(LIGHTBLUE "Pasażer [%d]: Statek jest pełny! Nie wchodzę na kładkę.\n", getpid());
                    exit(0);
                }

                semaphore_wait(sem_bridge, 0, 0);
                printf(LIGHTBLUE "Pasażer [%d]: Jest na kładce.\n", getpid());

                if (*koniec_wchodzenia == 1) 
                {
                    printf(LIGHTBLUE "Pasażer [%d]: Wchodzenie zostało zakończone, schodzę z kładki.\n", getpid());
                    semaphore_signal(sem_bridge, 0, 0);
                    exit(0);
                }

                // usleep(3000);

                printf(LIGHTBLUE "Pasażer [%d]: Próbuje wejść na statek...\n", getpid());
                if (*koniec_wchodzenia == 1) 
                {
                    printf(LIGHTBLUE "Pasażer [%d]: Wchodzenie zostało zakończone, schodzę z kładki.\n", getpid());
                    semaphore_signal(sem_bridge, 0, 0);
                    exit(0);
                }

                if (semctl(sem_ship, 0, GETVAL) > 0) 
                {
                    semaphore_wait(sem_ship, 0, 0); 
                    semaphore_signal(sem_bridge, 0, 0);
                    printf(LIGHTBLUE "Pasażer [%d]: Jest na statku.\n", getpid());

                    // zwiększam licznik pasazerow
                    __sync_fetch_and_add(shared_counter, 1);

                    if (semctl(sem_ship, 0, GETVAL) == 0) 
                    {
                        *ship_full_flag = 1;
                    }

                    // PID pasażera do pamięci współdzielonej
                    for (int j = 0; j < MAX_ON_SHIP; j++) 
                    {
                        if (pids_on_ship[j] == 0) 
                        {
                            pids_on_ship[j] = getpid();
                            break;
                        }
                    }
                    

                    pause(); 
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

        // Czekanie na sygnał powrotu statku
        struct message return_signal;
        printf(LIGHTBLUE "Pasażerowie: Czekam na sygnał od KapitanaStatku o powrocie do portu...\n");
        receive_message_from_queue(message_queue_ID, &return_signal, MSG_TYPE_RETURNED, 0);

        printf(LIGHTBLUE "\nPasażerowie: Rozpoczynam schodzenie ze statku.\n");

        // Schodzenie pasażerów NIE działa bez sleepa-> raz działa raz nie 
        for (int i = 0; i < MAX_ON_SHIP; i++) 
        {
            if (pids_on_ship[i] > 0) 
            {
                semaphore_wait(sem_bridge, 0, 0);
                printf(LIGHTBLUE "Pasażer [%d]: Schodzę ze statku...\n", pids_on_ship[i]);

                // Zmniejszenie licznika pasażerów
                __sync_fetch_and_sub(shared_counter, 1);

                usleep(1000);
                semaphore_signal(sem_bridge, 0, 0);
                kill(pids_on_ship[i], SIGUSR1); 
                pids_on_ship[i] = 0; 
            }
        }

        if(return_signal.content == 999)
        {
            rejs = R - 1;
        }
    }

    printf(RESET "\n=== Wszystkie rejsy zakończone ===\n");

    detach_shared_memory(koniec_wchodzenia, shared_end_boarding_id);

    destroy_semaphores(sem_bridge);
    destroy_semaphores(sem_ship);

    detach_shared_memory(pids_on_ship, shared_pid_mem_id);
    delete_shared_memory(shared_pid_mem_id);

    detach_shared_memory(ship_full_flag, shared_mem_id); 
    delete_shared_memory(shared_mem_id);

    semaphore_signal(sem_synchro, 0, 0); // semid, sem_num, flags

    reaper_running = false;
    pthread_join(reaper_tid, NULL);

    return 0;
}