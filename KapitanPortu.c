#include "funkcje_header.h"
#include <time.h>

#define MAX_REJSY 5 // Maksymalna liczba rejsów dziennie
#define T1 10       // Maksymalny czas oczekiwania przed odpłynięciem
#define T2 20       // Czas trwania rejsu

int main() 
{
    int message_queue_ID = initialize_message_queue(".", 'q', 0666 | IPC_CREAT);

    struct message signal_to_ship;
    signal_to_ship.type = 0; // Typ sygnału będzie różnicowany

    srand(time(NULL)); // Inicjalizacja generatora liczb losowych

    for (int rejs = 0; rejs < MAX_REJSY; rejs++) 
    {
        printf("Kapitan Portu: Rozpoczynam przygotowania do rejsu %d.\n", rejs + 1);

        // Decyzja losowa: czy wysłać sygnał 1 (odpłynięcie przed czasem)
        if (rand() % 2 == 0) 
        {
            printf("Kapitan Portu: Wysyłam sygnał 1 (nakaz odpłynięcia przed czasem).\n");
            signal_to_ship.type = 1;
            send_message_to_queue(message_queue_ID, &signal_to_ship, 0);
        }

        // Czas oczekiwania na odpłynięcie (T1) lub sygnał losowy
        sleep(T1);
        
        srand(time(NULL)); // Inicjalizacja generatora na podstawie aktualnego czasu
        // Decyzja losowa: czy wysłać sygnał 2 (przerwanie rejsu)
        if (((rand() % 14) + 1) % 7 == 0) 
        {
            printf("Kapitan Portu: Wysyłam sygnał 2 (przerwanie rejsu).\n");
            signal_to_ship.type = 2;
            send_message_to_queue(message_queue_ID, &signal_to_ship, 0);
        } 
        else 
        {
            printf("Kapitan Portu: Rejs odbywa się normalnie.\n");
        }

        // Czas trwania rejsu
        sleep(T2);
        printf("Kapitan Portu: Rejs %d zakończony.\n", rejs + 1);
    }

    printf("Kapitan Portu: Dzienny limit rejsów osiągnięty. Usuwam kolejkę komunikatów.\n");
    delete_message_queue(message_queue_ID);

    return 0;
}
