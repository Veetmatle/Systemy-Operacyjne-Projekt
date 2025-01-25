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

    key_t key_bridge       = ftok(".", 'b'); 
    key_t key_ship         = ftok(".", 's');
    key_t key_msg_queue    = ftok(".", 'k'); 
    key_t key_shared_pid   = ftok(".", 'p'); 
    key_t key_synchro_proc = ftok(".", 'u');
    key_t key_data         = ftok(".", 'm');  // Dodatkowy semafor (mutex) na dane

    if (key_bridge == -1 || key_ship == -1 || key_msg_queue == -1 
        || key_shared_pid == -1 || key_synchro_proc == -1 || key_data == -1) 
    {
        perror("Błąd ftok");
        exit(1);
    }

    // 1) Inicjalizacja semaforów
    int sem_bridge  = initialize_semaphores(key_bridge, 1);  // Ogranicza wejście na kładkę
    int sem_ship    = initialize_semaphores(key_ship, 1);    // Ogranicza liczbę miejsc na statku
    int sem_synchro = initialize_semaphores(key_synchro_proc, 1); // Do synchronizacji czyszczarki
    int sem_data    = initialize_semaphores(key_data, 1);    // MUTEX na dane

    // Semafor synchro używany gdzieś dalej:
    semctl(sem_synchro, 0, SETVAL, 0);

    // Ustawiamy wartość semafora mutex (do ochrony danych) na 1
    semctl(sem_data, 0, SETVAL, 1);

    // 2) Pamięć współdzielona: PID-y pasażerów
    int shared_pid_mem_id = initialize_shared_memory(".", 'p',
                                MAX_ON_SHIP * sizeof(pid_t), IPC_CREAT | 0666);
    pid_t *pids_on_ship = (pid_t *)attach_shared_memory(shared_pid_mem_id, NULL, 0);

    // 3) Pamięć współdzielona: flaga końca wchodzenia
    int shared_end_boarding_id = initialize_shared_memory(".", 'e',
                                sizeof(int), IPC_CREAT | 0666);
    int *koniec_wchodzenia = (int *)attach_shared_memory(shared_end_boarding_id, NULL, 0);

    // 4) Pamięć współdzielona: licznik pasażerów
    int shared_counter_id = initialize_shared_memory(".", 'c',
                                sizeof(int), IPC_CREAT | 0666);
    int *shared_counter = (int *)attach_shared_memory(shared_counter_id, NULL, 0);
    *shared_counter = 0;

    // 5) Pamięć współdzielona: flaga czy statek pełny
    int shared_mem_id = initialize_shared_memory(".", 'f',
                                sizeof(int), IPC_CREAT | 0666);
    int *ship_full_flag = (int *)attach_shared_memory(shared_mem_id, NULL, 0);
    *ship_full_flag = 0;

    // Wątek zbierający procesy-zombie
    pthread_t reaper_tid;
    if (pthread_create(&reaper_tid, NULL, reaper_thread, NULL) != 0) 
    {
        perror("Błąd tworzenia wątku");
        exit(1);
    }

    // ==================== Pętla rejsów ==================== 
    for (int rejs = 0; rejs < R; rejs++) 
    {
        *ship_full_flag = 0;
        *shared_counter = 0;
        *koniec_wchodzenia = 0;

        semctl(sem_bridge, 0, SETVAL, MAX_ON_BRIDGE);
        semctl(sem_ship, 0, SETVAL, MAX_ON_SHIP);

        for (int i = 0; i < MAX_ON_SHIP; i++) 
        {
            pids_on_ship[i] = 0;
        }

        // Oczekiwanie na sygnał zezwolenia od KapitanaStatku (MSG_TYPE_PERMISSION)
        struct message received_signal;
        printf(LIGHTBLUE "Pasażerowie: Czekam na sygnał od KapitanaStatku...\n");
        receive_message_from_queue(message_queue_ID, &received_signal, MSG_TYPE_PERMISSION, 0);

        printf(LIGHTBLUE "Pasażerowie: Otrzymali sygnał i rozpoczynają wsiadanie...\n");

        // ------------------- TWORZENIE PASAŻERÓW -------------------
        for (int i = 0; i < MAX_ON_SHIP + 10; i++) 
        {
            if (*koniec_wchodzenia == 1) 
            {
                break;
            }

            pid_t pid = fork(); 
            if (pid == -1)
            {
                perror("Fork failed");
                system("./clear_ipcs.sh");
                return -1;
            }

            if (pid == 0)
            {   
                // ------------------- KOD PROCESU-PASAŻERA -------------------
                printf(LIGHTBLUE "Pasażer [%d]: Próbuje wejść na kładkę...\n", getpid());

                // Najpierw sprawdzam, czy nie zakończono wchodzenia
                if (*koniec_wchodzenia == 1) 
                {
                    printf(LIGHTBLUE "Pasażer [%d]: Wchodzenie zakończone, nie wchodzę.\n", getpid());
                    exit(0);
                }

                // Statek pełny?
                if (*ship_full_flag == 1) 
                {
                    printf(LIGHTBLUE "Pasażer [%d]: Statek pełny! Rezygnuję.\n", getpid());
                    exit(0);
                }

                // Zajmuje semafor kładki:
                semaphore_wait(sem_bridge, 0, 0);
                printf(LIGHTBLUE "Pasażer [%d]: Jest na kładce.\n", getpid());

                // W sekcji krytycznej chroni sprawdzenie/czytanie i zapis:
                semaphore_wait(sem_data, 0, 0);

                    // Jeszcze raz sprawdzam, czy nie zakończono wchodzenia
                    if (*koniec_wchodzenia == 1) 
                    {
                        printf(LIGHTBLUE "Pasażer [%d]: Wchodzenie zakończone, schodzę z kładki.\n", getpid());
                        semaphore_signal(sem_data, 0, 0);
                        semaphore_signal(sem_bridge, 0, 0);
                        exit(0);
                    }

                    // Próba wejścia na statek (sprawdzenie dostępności sem_ship)
                    if (semctl(sem_ship, 0, GETVAL) <= 0)
                    {
                        // Brak miejsc:
                        printf(LIGHTBLUE "Pasażer [%d]: Wszystkie miejsca zajęte, rezygnuję.\n", getpid());
                        semaphore_signal(sem_data, 0, 0);
                        semaphore_signal(sem_bridge, 0, 0);
                        exit(0);
                    }

                    // Jest miejsce, to dekrementuje sem_ship:
                    semaphore_wait(sem_ship, 0, 0);

                    for (int j = 0; j < MAX_ON_SHIP; j++) 
                    {
                        if (pids_on_ship[j] == 0) 
                        {
                            pids_on_ship[j] = getpid();
                            break;
                        }
                    }

                    __sync_fetch_and_add(shared_counter, 1);

                    if (semctl(sem_ship, 0, GETVAL) == 0) 
                    {
                        *ship_full_flag = 1;
                    }

                // Koniec sekcji krytycznej
                semaphore_signal(sem_data, 0, 0);
                semaphore_signal(sem_bridge, 0, 0);

                printf(LIGHTBLUE "Pasażer [%d]: Jest już na statku.\n", getpid());

                // Teraz pasażer czeka na sygnał (SIGUSR1) oznaczający zejście
                pause(); 
                // Po SIGUSR1 -> proces umiera
                exit(0);
            }

            // usleep(10000); 
        }

        // ============= Oczekiwanie na powrót statku (MSG_TYPE_RETURNED) =============
        struct message return_signal;
        printf(LIGHTBLUE "Pasażerowie: Czekam na sygnał od KapitanaStatku o powrocie...\n");
        receive_message_from_queue(message_queue_ID, &return_signal, MSG_TYPE_RETURNED, 0);

        printf(LIGHTBLUE "\nPasażerowie: Rozpoczynam schodzenie ze statku.\n");

        // ============= Schodzenie pasażerów =============
        for (int i = 0; i < MAX_ON_SHIP; i++) 
        {
            if (pids_on_ship[i] > 0) 
            {
                // Najpierw trzeba wejść na kładkę
                semaphore_wait(sem_bridge, 0, 0);

                // Wejście do sekcji krytycznej (ochrona shared_counter i pids_on_ship)
                semaphore_wait(sem_data, 0, 0);

                    printf(LIGHTBLUE "Pasażer [%d]: Schodzi ze statku...\n", pids_on_ship[i]);

                    __sync_fetch_and_sub(shared_counter, 1);

                    pid_t child_pid = pids_on_ship[i];
                    pids_on_ship[i] = 0;

                semaphore_signal(sem_data, 0, 0);
                semaphore_signal(sem_bridge, 0, 0);

                // Dopiero teraz wysyłam sygnał SIGUSR1, żeby pasażer się zakończył
                kill(child_pid, SIGUSR1); 
            }
        }

        if(return_signal.content == 999)
        {
            rejs = R - 1;
        }
    }

    // ============= Koniec wszystkich rejsów =============
    printf(RESET "\n=== Wszystkie rejsy zakończone ===\n");

    // Sprzątanie
    detach_shared_memory(koniec_wchodzenia, shared_end_boarding_id);
    detach_shared_memory(pids_on_ship, shared_pid_mem_id);
    detach_shared_memory(ship_full_flag, shared_mem_id);

    delete_shared_memory(shared_pid_mem_id);
    delete_shared_memory(shared_mem_id);

    // Usunięcie semaforów do obsługi pasażerów
    destroy_semaphores(sem_bridge);
    destroy_semaphores(sem_ship);
    destroy_semaphores(sem_data);

    // Zwolnienie semafora synchro do czyszczary kapitana
    semaphore_signal(sem_synchro, 0, 0);

    reaper_running = false;
    pthread_join(reaper_tid, NULL);

    return 0;
}
