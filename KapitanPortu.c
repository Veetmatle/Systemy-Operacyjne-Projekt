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
            printf("KapitanPortu: Rozpoczynam zarządzanie portem.\n");
        }

        if (rejs != R - 1)
        {
            struct message received_signal;
            receive_message_from_queue(message_queue_ID, &received_signal, MSG_TYPE_PORT, 0);

            // Odczytanie PID z pola `content`
            pid_t received_pid = received_signal.content;
            printf("\nKapitanPortu: Otrzymano PID KapitanaStatku: %d\n", received_pid);
        

            int early_departure = rand() % 2;

            // wysyłam odpowiedni sygnał w zależności od decyzji
            if (early_departure) 
            {
                printf("KapitanPortu: Wysyłam sygnał wcześniejszego wypłynięcia (SIGUSR1).\n");
                kill(received_pid, SIGUSR1); // Wysłanie sygnału SIGUSR1
            } 
            else 
            {
                printf("KapitanPortu: Odpływanie normalnie.\n\n");
            }

            struct message signal_to_captain_you_can_leave;
            signal_to_captain_you_can_leave.type = MSG_TYPE_SIGNAL_TO_CAPTAIN_YOU_CAN_LEAVE;
            printf("KapitanPortu: Mozesz odpływać, szerokiej drogi! (...)\n\n");
            send_message_to_queue(message_queue_ID, &signal_to_captain_you_can_leave, 0);
        }
    }


    return 0;
}
