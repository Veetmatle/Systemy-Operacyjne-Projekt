#include "funkcje_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h> 

#define SEM_SHIP_DEPARTED   0
#define SEM_PORT_PROCESSED  1

int main() 
{
    // Najpierw wyczyść ewentualne stare semafory i kolejkę
    clear_existing_semaphores(".", 'F');
    clear_existing_message_queue(".", 'k');

    // Tworzymy dwa semafory (na indeksach 0 i 1)
    int sem_id = initialize_semaphores(ftok(".", 'F'), 2);

    // Tworzymy kolejkę komunikatów
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);

    srand(time(NULL));

    printf(RED "KapitanPortu: Rozpoczynam zarządzanie portem.\n");

    for (int rejs = 0; rejs < R; rejs++) 
    {
        // 1. Odbieramy info, że statek się załadował (MSG_TYPE_FIRST_PORT_SIGNAL)
        struct message received_signal_first_port;
        receive_message_from_queue(message_queue_ID, &received_signal_first_port, MSG_TYPE_FIRST_PORT_SIGNAL, 0);
        pid_t captain_pid = received_signal_first_port.content;
        
        // 2. Decyzja o przerwaniu rejsów (losowo 10%)
        int stop_cruises = (rand() % 10 == 0); // 10% szans
        if (stop_cruises) 
        {
            printf(RED "\nKapitanPortu: DECYZJA: Przerywamy rejsy - wysyłam sygnał SIGUSR2!\n");
            kill(captain_pid, SIGUSR2);
        }

        semaphore_signal(sem_id, SEM_PORT_PROCESSED, 0);    

        if (stop_cruises)
        {
            printf(RED "KapitanPortu: Rejsy przerwane, kończę pracę.\n");
            break; // wyjście z pętli for
        }

        // Tutaj obsługa wysłania kolejnego sygnału o zakonczeniu rejsow - tym razem w petli juz
        printf(RED "KapitanPortu: Oczekuję na odpłynięcie statku (SEM_SHIP_DEPARTED)\n");
        semaphore_wait(sem_id, SEM_SHIP_DEPARTED, 0);

        stop_cruises = (rand() % 10 == 1);
        if (stop_cruises) 
        {
            printf(RED "\nKapitanPortu: DECYZJA: Przerywamy rejsy - wysyłam sygnał SIGUSR2!\n");
            kill(captain_pid, SIGUSR2);
        }  

        semaphore_signal(sem_id, SEM_PORT_PROCESSED, 0);

        if (stop_cruises)
        {
            printf(RED "KapitanPortu: Rejsy przerwane, kończę pracę.\n");
            break; 
        }
    
        // Jeśli to nie ostatni rejs, obsługujemy sygnał w stylu: kolejny załadunek
        if (rejs != R - 1)
        {
            struct message received_signal;
            receive_message_from_queue(message_queue_ID, &received_signal, MSG_TYPE_PORT, 0);
            printf(RED "\nKapitanPortu: Otrzymano sygnał od KapitanaStatku (PID: %d)\n", received_signal.content);
        
            int early_departure = ((rand() % 10) < 3);
            if (early_departure) 
            {
                printf(RED "KapitanPortu: DECYZJA: Zarządzam wcześniejsze wypłynięcie - wysyłam SIGUSR1!\n");
                kill(captain_pid, SIGUSR1);
            } 
            else 
            {
                printf(RED "KapitanPortu: DECYZJA: Statek wypłynie zgodnie z normalnym harmonogramem.\n");
            }

            struct message departure_permission;
            departure_permission.type = MSG_TYPE_SIGNAL_TO_CAPTAIN_YOU_CAN_LEAVE;
            printf(RED "KapitanPortu: Wydaję zgodę na odpłynięcie. Szerokiej drogi!\n\n");
            send_message_to_queue(message_queue_ID, &departure_permission, 0);
        }
    }
    
    return 0;
}
