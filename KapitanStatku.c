#include "funkcje_header.h"

struct timer_args 
{
    int time_limit;        // Czas w mikrosekundach
    volatile bool *timer_finished;  
};

void* timer_thread(void* arg) 
{
    struct timer_args *args = (struct timer_args*)arg;
    int elapsed_time = 0; 

    // Timer co 100 ms do kontroli flagi
    while (elapsed_time < args->time_limit) 
    {
        if (*(args->timer_finished)) 
        {
            return NULL;
        }
        usleep(100000); // 100 ms
        elapsed_time += 100000;
    }

    *(args->timer_finished) = true; 
    return NULL;
}

/* reszta inicjalizacji */
int wczesniejsze_odplywanie = 0; // 0 - normalne 1 - szybsze
int koniec_rejsow = 0;

void handle_early_departure(int sig) 
{
    wczesniejsze_odplywanie = 1;
}

void handle_end_of_cruises(int sig) 
{
    koniec_rejsow = 1;
}

int main() 
{
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);
    signal(SIGUSR1, handle_early_departure);  
    signal(SIGUSR2, handle_end_of_cruises);  

    int sem_id = semget(ftok(".", 'F'), 2, 0666);
    if (sem_id == -1) 
    {
        perror("Błąd dostępu do semaforów");
        exit(1);
    }

    // Inicjalizacja semafora do synchro
    key_t key_synchro_proces = ftok(".", 'u');
    int sem_synchro = initialize_semaphores(key_synchro_proces, 1);

    // Inicjalizacja pamięci współdzielonej dla licznika pasażerów
    int shared_counter_id = initialize_shared_memory(".", 'c', sizeof(int), IPC_CREAT | 0666);
    int *shared_counter = (int *)attach_shared_memory(shared_counter_id, NULL, 0);

    // Inicjalizacja pamięci współdzielonej dla flagi koniec_wchodzenia
    int shared_end_boarding_id = initialize_shared_memory(".", 'e', sizeof(int), IPC_CREAT | 0666);
    int *koniec_wchodzenia = (int *)attach_shared_memory(shared_end_boarding_id, NULL, 0);
    *koniec_wchodzenia = 0;  

    for (int rejs = 0; rejs < R; rejs++) 
    {
        printf(RESET "\n=== Parostatkiem w piękny rejs numer %d ===\n", rejs + 1);  

        if (rejs == 0)
        {
            struct message signal_to_passengers;
            signal_to_passengers.type = MSG_TYPE_PERMISSION;
            signal_to_passengers.content = 1; // "zezwalam"
            
            printf(GREEN "KapitanStatku: Przygotowuję statek do wsiadania pasażerów...\n");

            printf(GREEN "KapitanStatku: Wysyłam sygnał do pasażerów: Można wchodzić na statek.\n");
            send_message_to_queue(message_queue_ID, &signal_to_passengers, 0);
        }

        printf(GREEN "KapitanStatku: Czekam, aż pasażerowie wejdą na statek lub minie czas oczekiwania.\n");

        // wątek odmierzający czas
        volatile bool timer_finished = false;
        pthread_t timer_tid;
        struct timer_args args;

        args.time_limit = (wczesniejsze_odplywanie == 1) ? (T1/2) : T1;
        args.timer_finished = &timer_finished;

        if (pthread_create(&timer_tid, NULL, timer_thread, &args) != 0) 
        {
            perror("Błąd tworzenia wątku timera");
            exit(1);
        }

        while (1) 
        {
            if (*shared_counter == MAX_ON_SHIP || timer_finished) 
            {
                if (*shared_counter == MAX_ON_SHIP) 
                {
                    printf(GREEN "KapitanStatku: Statek zapełniony (%d osób).\n", *shared_counter);
                } else if (*shared_counter == 0 && timer_finished) 
                {
                    printf(GREEN "KapitanStatku: Upłynął czas oczekiwania. Rozpoczynam rejs bez pasażerów.\n");
                } else if (wczesniejsze_odplywanie == 1) 
                {
                    printf(GREEN "KapitanStatku: Wcześniejsze odpłynięcie - kończymy wsiadanie przy %d pasażerach.\n", 
                        *shared_counter);
                } else 
                {
                    printf(GREEN "KapitanStatku: Upłynął maksymalny czas oczekiwania. Kończymy wsiadanie przy %d pasażerach.\n", 
                        *shared_counter);
                }
                *koniec_wchodzenia = 1;
                if (!timer_finished) 
                {
                    timer_finished = true;
                }
                break;
            }
            // usleep(50000); // 0.05 sek przerwy
        }

        pthread_join(timer_tid, NULL);

        // reset wczesniejszego odplywania
        wczesniejsze_odplywanie = 0; 

        // Informuje KapitanaPortu (kolejką) o tym, że pasażerowie są na pokładzie
        struct message first_signal_to_port;
        first_signal_to_port.type = MSG_TYPE_FIRST_PORT_SIGNAL;
        first_signal_to_port.content = getpid(); 
        printf(GREEN "KapitanStatku: Informacja dla portu, pasażerowie weszli na statek!\n");
        send_message_to_queue(message_queue_ID, &first_signal_to_port, 0);

        // Czekam az port to przetrawi
        semaphore_wait(sem_id, SEM_PORT_PROCESSED, 0);

        // Jezeli przerwanie to
        if (koniec_rejsow == 1)
        {
            printf(GREEN "KapitanStatku: Rejs się nie odbędzie. Proszę wyjść.\n");
            printf(GREEN "KapitanStatku: Czekam, aż wszyscy pasażerowie zejdą...\n");

            struct message return_signal;
            return_signal.type = MSG_TYPE_RETURNED;
            return_signal.content = 999;
            send_message_to_queue(message_queue_ID, &return_signal, 0);

            while (1)
            {
                if (*shared_counter == 0)
                {
                    printf(GREEN "KapitanStatku: Wszyscy pasażerowie zeszli ze statku.\n");
                    break;
                }
                // usleep(50000);
            }

            delete_message_queue(message_queue_ID);
            printf(GREEN "KapitanStatku: Usuwam kolejkę komunikatów i kończę operację.\n");
            break; 
        }

        // Jeżeli rejs ma się odbyć:
        printf(GREEN "KapitanStatku: Wsiadanie zakończone. Odpływam za 10 sekund...\n");
        // usleep(10000);

        printf(GREEN "KapitanStatku: Statek odpływa...\n");
        printf("KapitanStatku: Sygnalizuję odpłynięcie (SEM_SHIP_DEPARTED)\n");
        semaphore_signal(sem_id, SEM_SHIP_DEPARTED, 0);

        semaphore_wait(sem_id, SEM_PORT_PROCESSED, 0);

        // Jeszcze raz sprawdzam sygnał przerwania (w trakcie rejsu juz)
        if(koniec_rejsow == 1)
        {
            rejs = R - 1; 
            printf(GREEN "KapitanStatku: Otrzymałem sygnał przerwania rejsów! Dokończę bieżący rejs i koniec...\n");
        }

        // Symulacja rejsu (chwilwo 10 pseudo iteracjo sekund zeby nie bylo)
        for (int i = 0; i < 10; i++) 
        {
            printf(RESET "KapitanStatku: Statek w drodze... %d sekund\n", i + 1);
            usleep(1000);
        }
        printf(GREEN "\nKapitanStatku: Statek zakończył rejs.\n");

        // dam tutaj sygnal pasazerom ze to ostatni rejs w razie gdyby bylo przerwanie sygnalem podczas rejsu
        if (rejs == R - 1)
        {
            struct message return_signal;
            return_signal.type = MSG_TYPE_RETURNED;
            return_signal.content = 999;
            send_message_to_queue(message_queue_ID, &return_signal, 0);
        }
        else
        {
            struct message return_signal;
            return_signal.type = MSG_TYPE_RETURNED;
            return_signal.content = 1;
            printf(GREEN "KapitanStatku: Wysłano sygnał do pasażerów: Statek wrócił do portu.\nCzekam, aż wszyscy pasażerowie zejdą...\n");
            send_message_to_queue(message_queue_ID, &return_signal, 0);
        }
        
        while (1)
        {
            if (*shared_counter == 0)
                {
                    printf(GREEN "KapitanStatku: Wszyscy pasażerowie zeszli ze statku.\n");
                    break;
                }
            // usleep(50000); 
        }

        // Jeżeli to nie był ostatni rejs – kolejny załadunek
        if (rejs != R - 1)
        {
            *koniec_wchodzenia = 0;

            struct message signal_to_passengers;
            signal_to_passengers.type = MSG_TYPE_PERMISSION; 
            signal_to_passengers.content = 1;              
            printf(GREEN "KapitanStatku: Wysyłam sygnał do pasażerów: Można wchodzić na statek.\n");
            send_message_to_queue(message_queue_ID, &signal_to_passengers, 0);

            // Info do KapitanaPortu, że statek jest gotowy do kolejnego rejsu
            struct message signal_to_port;
            signal_to_port.type = MSG_TYPE_PORT;
            signal_to_port.content = getpid();
            printf(GREEN "KapitanStatku: Informacja dla portu: statek wrócił z rejsu...\n\n");
            send_message_to_queue(message_queue_ID, &signal_to_port, 0);

            // Czekam na wiadomość w kolejce od Portu (pozwolenie na odpłynięcie)
            struct message captain_can_leave;
            printf(GREEN "KapitanStatku: Czekam na pozwolenie od KapitanaPortu na odpłynięcie...\n");
            receive_message_queue_antyprzerwanie(message_queue_ID, MSG_TYPE_SIGNAL_TO_CAPTAIN_YOU_CAN_LEAVE, &captain_can_leave);
            printf(GREEN "KapitanStatku: Otrzymano sygnał, że statek gotowy do rejsu.\n");

            // Sprawdzam, czy nie było sygnału SIGUSR1 (wcześniejsze wypłynięcie)
            if(wczesniejsze_odplywanie == 1)
            {
                printf(GREEN "KapitanStatku: Otrzymano sygnał wcześniejszego wypłynięcia (SIGUSR1).\n");
                printf(GREEN "KapitanStatku: Odpływam niebawem!\n");
            }
            else
            {
                printf(GREEN "\nKapitanStatku: Otrzymano polecenie normalnego wypłynięcia..\n");
                printf(GREEN "KapitanStatku: Czekam %d sekund przed rozpoczęciem rejsu...\n", T1);
            }
        }

        if (rejs == R - 1) 
        {
            delete_message_queue(message_queue_ID);
            printf(GREEN "KapitanStatku: Usuwam kolejkę komunikatów i kończę operację.\n");
        }
    }

    semaphore_wait(sem_synchro, 0, 0); 
    printf("DOSTAŁEM SEMAFOR ZE SIE ZAKONCZYLI KOM DO DEBUGOWANIAA\n");
    destroy_semaphores(sem_synchro);

    detach_shared_memory(shared_counter, shared_counter_id);
    delete_shared_memory(shared_counter_id);

    detach_shared_memory(koniec_wchodzenia, shared_end_boarding_id);
    delete_shared_memory(shared_end_boarding_id);

    destroy_semaphores(sem_id);

    return 0;
}
