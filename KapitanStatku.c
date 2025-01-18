#include "funkcje_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

// wywalic semafora, dodac kolejke komunikatow po wyslaniu sygnalu

#define MSG_TYPE_PERMISSION 1 // Typ wiadomości dla zezwolenia na wejście na statek
#define MSG_TYPE_RETURNED 10   // Typ wiadomości dla powrotu statku do portu
#define T2 5                 // Czas trwania rejsu w sekundach            
#define T1 60                 // Statek odpływa co T1   
#define SEMAPHORE_KEY 1234 // Klucz dla semafora

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
    // clear_existing_message_queue(".", 'k'); <- to powinien miec pierwszy proces kroty sie odpala
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);
    signal(SIGUSR1, handle_early_departure);
    signal(SIGUSR2, handle_end_of_cruises);

    for (int rejs = 0; rejs < R; rejs++) 
    {
        printf(RESET "\n=== Parostatkiem w piękny rejs numer %d ===\n", rejs + 1);

        wczesniejsze_odplywanie = 0;

        if (rejs == 0)
        {
            struct message signal_to_passengers;
            signal_to_passengers.type = MSG_TYPE_PERMISSION; // Typ wiadomości (zezwolenie)
            signal_to_passengers.content = 1;               // Treść wiadomości (np. kod "zezwalam")
            

            printf(GREEN "KapitanStatku: Przygotowuję statek do wsiadania pasażerów...\n");
            usleep(3000); 

            printf(GREEN "KapitanStatku: Wysyłam sygnał do pasażerów: Można wchodzić na statek.\n");
            send_message_to_queue(message_queue_ID, &signal_to_passengers, 0);
        }

        // Odbiór wiadomosci zakończenia wsiadania
        struct message received_end_signal;
        printf(GREEN "KapitanStatku: Czekam na sygnał o zakończeniu wsiadania pasażerów...\n");
        receive_message_from_queue(message_queue_ID, &received_end_signal, 2, 0);

        printf(GREEN "\nKapitanStatku: Otrzymałem info, że wszyscy pasażerowie są na statku. Przygotowuję się do rejsu.\n");

        // wiadomosc do kapitana portu z PIDEM przed losowaniem (czy przerwanie, czy schodzą ze statku)
        struct message first_signal_to_port;
        first_signal_to_port.type = MSG_TYPE_FIRST_PORT_SIGNAL;
        first_signal_to_port.content = getpid();
        printf(GREEN "KapitanStatku: Informacja dla portu, pasażerowie weszli na statek!\n\n");
        send_message_to_queue(message_queue_ID, &first_signal_to_port, 0);

        // czekanie na obsługę pierwszego sygnału (czy konczymy rejsy i pasazerownie schodzą ze statku) przez ultra kolejke komunikatow
        struct message end_of_port;
        // printf(GREEN "KapitanStatku: Czekam na wiadomomość że ewentualny sygnał wysłany.\n");
        receive_message_queue_antyprzerwanie(message_queue_ID, MSG_TYPE_END_OF_PORT, &end_of_port);

        // ewentualna obsługa przerwania rejsów w ifie (sygnał)
        if (koniec_rejsow == 1)
        {
            printf(GREEN "KapitanStatku: Wysłano sygnał do pasażerów: Rejs się nie odbędzie. Proszę wyjść.\n");

            // kod do zakonczenia rejsow
            struct message return_signal;
            return_signal.type = MSG_TYPE_RETURNED;
            return_signal.content = 999;
            send_message_to_queue(message_queue_ID, &return_signal, 0);

            // pasazerowie zakonczyli opuszczanie statku, info o zakonczeniu schodzenia pasazerow
            struct message passengers_disembarked_signal;
            printf(GREEN "KapitanStatku: Czekam na sygnał, że wszyscy pasażerowie zeszli ze statku...\n");
            receive_message_from_queue(message_queue_ID, &passengers_disembarked_signal, 3, 0);
            printf(GREEN "KapitanStatku: Otrzymano sygnał, że wszyscy pasażerowie zeszli ze statku.\n");

            delete_message_queue(message_queue_ID);
            printf(GREEN "KapitanStatku: Usuwam kolejkę komunikatów i kończę operację.\n");

            break;
        }

        

        // Odczekanie 10 sekund przed odpłynięciem (zamykanie wsiadania)
        printf(GREEN "KapitanStatku: Wsiadanie zakończone. Odpływam za 10 sekund...\n");
        usleep(10000);

        // Symulacja rejsu
        printf(GREEN "KapitanStatku: Statek odpływa...\n");
        for (int i = 0; i < T2; i++) 
        {
            printf(RESET "KapitanStatku: Statek w drodze... %d sekund\n", i + 1);
            usleep(1000);
        }
        printf(GREEN "\nKapitanStatku: Statek zakończył rejs.\n");

        // Wysłanie wiadomości o powrocie statku do portu
        struct message return_signal;
        return_signal.type = MSG_TYPE_RETURNED;
        return_signal.content = 1;
        send_message_to_queue(message_queue_ID, &return_signal, 0);
        printf(GREEN "KapitanStatku: Wysłano sygnał do pasażerów: Statek wrócił do portu.\n");

        // pasazerowie zakonczyli opuszczanie statku, info o zakonczeniu schodzenia pasazerow
        struct message passengers_disembarked_signal;
        printf(GREEN "KapitanStatku: Czekam na sygnał, że wszyscy pasażerowie zeszli ze statku...\n");
        receive_message_from_queue(message_queue_ID, &passengers_disembarked_signal, 3, 0);
        printf(GREEN "KapitanStatku: Otrzymano sygnał, że wszyscy pasażerowie zeszli ze statku.\n");

        if (rejs != R - 1)
        {
            // wiadomosc do pasazerow, mozna wchodzic od nowa
            struct message signal_to_passengers;
            signal_to_passengers.type = MSG_TYPE_PERMISSION; 
            signal_to_passengers.content = 1;              
            printf(GREEN "KapitanStatku: Wysyłam sygnał do pasażerów: Można wchodzić na statek.\n");
            send_message_to_queue(message_queue_ID, &signal_to_passengers, 0);

            // wiadomosc do kapitana portu z PIDEM przed losowaniem
            struct message signal_to_port;
            signal_to_port.type = MSG_TYPE_PORT;
            signal_to_port.content = getpid();
            printf(GREEN "KapitanStatku: Informacja dla portu, statek wrócił z rejsu...\n\n");
            send_message_to_queue(message_queue_ID, &signal_to_port, 0);

            // czekanie na obsługę sygnału przez ultra kolejke komunikatow
            struct message captain_can_leave;
            printf(GREEN "KapitanStatku: Czekam na pozwolenie od KapitanaStatku na odpłynięcie...\n");
            receive_message_queue_antyprzerwanie(message_queue_ID, MSG_TYPE_SIGNAL_TO_CAPTAIN_YOU_CAN_LEAVE, &captain_can_leave);
            printf(GREEN "KapitanStatku: Otrzymano sygnał, że statek gotowy do rejsu.\n");

            // kod po wykonaniu sygnału (musze dodac jakies opoznienie, petla dla zmiennej glob wywalila program)
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

        // Na zakończenie ostatniego rejsu usuń kolejkę komunikatów
        if (rejs == R - 1) 
        {
            delete_message_queue(message_queue_ID);
            printf(GREEN "KapitanStatku: Usuwam kolejkę komunikatów i kończę operację.\n");
        }
    }

    return 0;
}