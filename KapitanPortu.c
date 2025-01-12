#include "funkcje_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 

int main() 
{
    srand(time(NULL)); // do losowania

    clear_existing_message_queue(".", 'k');
    int message_queue_ID = initialize_message_queue(".", 'k', 0666 | IPC_CREAT);

    for (int rejs = 0; rejs < R; rejs++) 
    {
        if (rejs == 0)
        {
            printf("KapitanPortu: Rozpoczynam zarządzanie portem.\n");
        }

        int early_departure = rand() % 2;
        if (early_departure) 
        {
            struct message departure_signal;
            departure_signal.type = MSG_TYPE_EARLY_DEPARTURE;
            departure_signal.content = 99;
            send_message_to_queue(message_queue_ID, &departure_signal, 0);
        } 
        else 
        {
            // Wysłanie sygnału o normalnym wypłynięciu
            struct message normal_departure_signal;
            normal_departure_signal.type = MSG_TYPE_NORMAL_DEPARTURE;
            normal_departure_signal.content = 1;
            send_message_to_queue(message_queue_ID, &normal_departure_signal, 0);
        }

        if (rejs != R - 1)
        {
            struct message port_signal;
            receive_message_from_queue(message_queue_ID, &port_signal, MSG_TYPE_PORT, 0);
            printf("KapitanPortu: monitoruję sytuację.\n");
        }
    }


    return 0;
}
