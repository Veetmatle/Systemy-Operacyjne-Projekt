#include "funkcje_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 

int main() 
{
    clear_existing_message_queue(".", 'k');
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);
    srand(time(NULL));

    for (int rejs = 0; rejs < R; rejs++) 
    {
        if (rejs == 0)
        {
            printf(RED "KapitanPortu: Rozpoczynam zarządzanie portem.\n");
        }

        // Obsługa sygnału przerwania rejsów (podczas załadunku)
        struct message received_signal_first_port;
        receive_message_from_queue(message_queue_ID, &received_signal_first_port, MSG_TYPE_FIRST_PORT_SIGNAL, 0);
        pid_t captain_pid = received_signal_first_port.content;
        
        // Zwiększamy szansę na przerwanie rejsów dla lepszego testowania
        int stop_cruises = (rand() % 5 == 0); // 20% szansa na przerwanie

        if (stop_cruises) 
        {
            printf(RED "\nKapitanPortu: DECYZJA: Przerywamy rejsy - wysyłam sygnał SIGUSR2!\n");
            kill(captain_pid, SIGUSR2);
        }    

        // Potwierdzenie przetworzenia sygnału
        struct message confirmation;
        confirmation.type = MSG_TYPE_END_OF_PORT;
        send_message_to_queue(message_queue_ID, &confirmation, 0);

        if (stop_cruises)
        {
            printf(RED "KapitanPortu: Rejsy przerwane, kończę pracę.\n");
            break;
        }
            
        if (rejs != R - 1)
        {
            // Obsługa sygnału wcześniejszego wypłynięcia
            struct message received_signal;
            receive_message_from_queue(message_queue_ID, &received_signal, MSG_TYPE_PORT, 0);
            printf(RED "\nKapitanPortu: Otrzymano sygnał od KapitanaStatku (PID: %d)\n", received_signal.content);
        
            // 30% szansa na wcześniejsze wypłynięcie
            int early_departure = (rand() % 10) < 3;

            if (early_departure) 
            {
                printf(RED "KapitanPortu: DECYZJA: Zarządzam wcześniejsze wypłynięcie - wysyłam SIGUSR1!\n");
                kill(captain_pid, SIGUSR1);
            } 
            else 
            {
                printf(RED "KapitanPortu: DECYZJA: Statek wypłynie zgodnie z normalnym harmonogramem.\n");
            }

            // Zezwolenie na odpłynięcie
            struct message departure_permission;
            departure_permission.type = MSG_TYPE_SIGNAL_TO_CAPTAIN_YOU_CAN_LEAVE;
            printf(RED "KapitanPortu: Wydaję zgodę na odpłynięcie. Szerokiej drogi!\n\n");
            send_message_to_queue(message_queue_ID, &departure_permission, 0);

            // ! Tutaj obsługa wysłania znów sygnału o przerywaniu rejsu...
        }
    }

    return 0;
}