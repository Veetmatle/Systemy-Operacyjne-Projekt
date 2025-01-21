#include "funkcje_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define SEM_SHIP_DEPARTED   0
#define SEM_PORT_PROCESSED  1

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

    // Dostęp do semaforów (2 sztuki):
    int sem_id = semget(ftok(".", 'F'), 2, 0666);
    if (sem_id == -1) 
    {
        perror("Błąd dostępu do semaforów");
        exit(1);
    }

    // Inicjalizacja pamięci współdzielonej – PID-y osób na statku
    int shared_pid_mem_id = initialize_shared_memory(".", 'p', MAX_ON_SHIP * sizeof(pid_t), IPC_CREAT | 0666);
    pid_t *pids_on_ship   = (pid_t *)attach_shared_memory(shared_pid_mem_id, NULL, 0);

    for (int rejs = 0; rejs < R; rejs++) 
    {
        printf(RESET "\n=== Parostatkiem w piękny rejs numer %d ===\n", rejs + 1);
        wczesniejsze_odplywanie = 0;  

        if (rejs == 0)
        {
            struct message signal_to_passengers;
            signal_to_passengers.type = MSG_TYPE_PERMISSION;
            signal_to_passengers.content = 1; // "zezwalam"
            
            printf(GREEN "KapitanStatku: Przygotowuję statek do wsiadania pasażerów...\n");
            usleep(3000); 

            printf(GREEN "KapitanStatku: Wysyłam sygnał do pasażerów: Można wchodzić na statek.\n");
            send_message_to_queue(message_queue_ID, &signal_to_passengers, 0);
        }

        printf(GREEN "KapitanStatku: Czekam, aż statek będzie pełny (pids_on_ship się zapełni)...\n");
        while (1)
        {
            int count = 0;
            for (int i = 0; i < MAX_ON_SHIP; i++)
            {
                if (pids_on_ship[i] != 0)
                    count++;
            }

            if (count == MAX_ON_SHIP)
            {
                printf(GREEN "KapitanStatku: Dobra, widzę że wszyscy się zapakowali na statek...\n");
                break;
            }
            usleep(50000); // 0.05 sek
        }

        // Informujemy KapitanaPortu (kolejką) o tym, że pasażerowie są na pokładzie
        struct message first_signal_to_port;
        first_signal_to_port.type = MSG_TYPE_FIRST_PORT_SIGNAL;
        first_signal_to_port.content = getpid(); // PID statku
        printf(GREEN "KapitanStatku: Informacja dla portu, pasażerowie weszli na statek!\n");
        send_message_to_queue(message_queue_ID, &first_signal_to_port, 0);

        // Oczekuj, aż Port „przetworzy” (decyzja o przerwaniu/nieprzerwaniu) -> SEM_PORT_PROCESSED
        semaphore_wait(sem_id, SEM_PORT_PROCESSED, 0);

        // Sprawdź, czy port nie zdecydował o przerwaniu rejsów (SIGUSR2)
        if (koniec_rejsow == 1)
        {
            printf(GREEN "KapitanStatku: Rejs się nie odbędzie. Proszę wyjść.\n");
            printf(GREEN "KapitanStatku: Czekam, aż wszyscy pasażerowie zejdą...\n");

            // Powiadamiamy pasażerów o zejściu
            struct message return_signal;
            return_signal.type = MSG_TYPE_RETURNED;
            return_signal.content = 999;
            send_message_to_queue(message_queue_ID, &return_signal, 0);

            while (1)
            {
                int count = 0;
                for (int i = 0; i < MAX_ON_SHIP; i++)
                    if (pids_on_ship[i] != 0) count++;

                if (count == 0)
                {
                    printf(GREEN "KapitanStatku: Wszyscy pasażerowie zeszli ze statku.\n");
                    break;
                }
                usleep(50000); 
            }

            delete_message_queue(message_queue_ID);
            printf(GREEN "KapitanStatku: Usuwam kolejkę komunikatów i kończę operację.\n");
            break; // przerwanie całej pętli rejsów
        }

        // Jeżeli rejs ma się odbyć:
        printf(GREEN "KapitanStatku: Wsiadanie zakończone. Odpływam za 10 sekund...\n");
        usleep(10000);

        printf(GREEN "KapitanStatku: Statek odpływa...\n");
        // Sygnalizujemy Portowi, że rzeczywiście wypływamy
        printf("KapitanStatku: Sygnalizuję odpłynięcie (SEM_SHIP_DEPARTED)\n");
        semaphore_signal(sem_id, SEM_SHIP_DEPARTED, 0);

        // Czekam, aż port zakończy przetwarzanie tego etapu
        semaphore_wait(sem_id, SEM_PORT_PROCESSED, 0);

        // Jeszcze raz sprawdzamy sygnał przerwania (np. w trakcie rejsu)
        if(koniec_rejsow == 1)
        {
            rejs = R - 1; // kończymy pętlę szybciej
            printf(GREEN "KapitanStatku: Otrzymałem sygnał przerwania rejsów! Dokończę bieżący rejs i koniec...\n");
        }

        // Symulacja rejsu
        for (int i = 0; i < T2; i++) 
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
        
        // Czekam faktycznie aż zapełniona tablica PID-ów się wyzeruje
        while (1)
        {
            int count = 0;
            for (int i = 0; i < MAX_ON_SHIP; i++)
                if (pids_on_ship[i] != 0) count++;

            if (count == 0)
            {
                printf(GREEN "KapitanStatku: Wszyscy pasażerowie zeszli ze statku.\n");
                break;
            }
            usleep(50000); 
        }

        // Jeżeli to nie był ostatni rejs – kolejny załadunek
        if (rejs != R - 1)
        {
            // Zezwolenie dla pasażerów na wsiadanie
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

            // Sprawdzam, czy nie było sygnału SIGUSR1 (wcześniejsze wypłynięcie) lub SIGUSR2
            if(wczesniejsze_odplywanie == 1)
            {
                printf(GREEN "KapitanStatku: Otrzymano sygnał wcześniejszego wypłynięcia (SIGUSR1).\n");
                printf(GREEN "KapitanStatku: Odpływam niebawem!\n");
            }
            else
            {
                printf(GREEN "\nKapitanStatku: Otrzymano sygnał normalnego wypłynięcia (SIGUSR2).\n");
                printf(GREEN "KapitanStatku: Czekam %d sekund przed rozpoczęciem rejsu...\n", T1);
                usleep(T1 * 1000);  
            }
        }

        // Jeśli to ostatni rejs, to usuwamy kolejkę
        if (rejs == R - 1) 
        {
            delete_message_queue(message_queue_ID);
            printf(GREEN "KapitanStatku: Usuwam kolejkę komunikatów i kończę operację.\n");
        }
    }

    // Na koniec usuwamy semafory (jeśli tak zakładamy w logice – trzeba upewnić się,
    // że KapitanPortu też już zakończył, w przeciwnym razie on straci semafory).
    destroy_semaphores(sem_id);

    return 0;
}
