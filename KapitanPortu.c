#include "funkcje_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 

int main() 
{
    clear_existing_message_queue(".", 'k');
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);
    srand(time(NULL)); // do losowania

    for (int rejs = 0; rejs < R; rejs++) 
    {
        if (rejs == 0)
        {
            printf(RED "KapitanPortu: Rozpoczynam zarządzanie portem.\n");
        }

            // SYGNAŁ 1 OBSŁUGA
        struct message received_signal_first_port;
        receive_message_from_queue(message_queue_ID, &received_signal_first_port, MSG_TYPE_FIRST_PORT_SIGNAL, 0);
        pid_t received_first_pid = received_signal_first_port.content;
        int end_of_cruises = rand() % 10 + 1;

        if (end_of_cruises == 1) 
        {
            printf(RED "\nKapitanPortu: Nakazuję przerwanie rejsów! Statek kończy rejsy!\n");
            kill(received_first_pid, SIGUSR2);
        }    

        // potwierdzenie sygnału do kap statku
        struct message send_signal_first_port;
        send_signal_first_port.type = MSG_TYPE_END_OF_PORT;
        send_message_to_queue(message_queue_ID, &send_signal_first_port, 0);

        if (end_of_cruises == 1)
        {
            printf(RED "KapitanPortu: Pracę wykonałem, zawijam do domu.\n");
            break;
        }
            
        if (rejs != R - 1)
        {
            // SYGNAŁ 2 OBSŁUGA 
            struct message received_signal;
            receive_message_from_queue(message_queue_ID, &received_signal, MSG_TYPE_PORT, 0);
            pid_t received_pid = received_signal.content;
            printf(RED "\nKapitanPortu: Otrzymano PID KapitanaStatku: %d\n", received_pid);
        
            int early_departure = rand() % 2;

            // wysyłam odpowiedni sygnał w zależności od decyzji
            if (early_departure) 
            {
                printf(RED "KapitanPortu: DECYZJA: Wysyłam sygnał wcześniejszego wypłynięcia (SIGUSR1).\n");
                kill(received_pid, SIGUSR1); // Wysłanie sygnału SIGUSR1
            } 
            else 
            {
                printf(RED "KapitanPortu: DECYZJA: Odpływanie normalnie.\n");
            }

            struct message signal_to_captain_you_can_leave;
            signal_to_captain_you_can_leave.type = MSG_TYPE_SIGNAL_TO_CAPTAIN_YOU_CAN_LEAVE;
            printf(RED "KapitanPortu: Statek ma zielone światło, szykujcie się do odpłynięcia, szerokiej drogi! (...)\n\n");
            send_message_to_queue(message_queue_ID, &signal_to_captain_you_can_leave, 0);

            // SYGNAL 1 OBSLUGA W CZASIE REJSU

        }
    }


    return 0;
}
