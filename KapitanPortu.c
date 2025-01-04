#include "funkcje_header.h"

#define MAX_REJSY 5 // Maksymalna liczba rejsów dziennie
#define T1 10       // Czas maksymalny przed odpłynięciem
#define T2 20       // Czas trwania rejsu

int main() {
    int message_queue_ID = initialize_message_queue(".", 'q', 0666 | IPC_CREAT);

    struct message signal_to_ship;
    signal_to_ship.type = 0; // Sygnały będą różnicowane na podstawie typu

    for (int rejs = 0; rejs < MAX_REJSY; rejs++) {
        printf("Kapitan Portu: Wysyłam sygnał 1 (rozpoczęcie rejsu).\n");
        signal_to_ship.type = 1;
        send_message_to_queue(message_queue_ID, &signal_to_ship, 0);

        // Symulacja czasu odpłynięcia i trwania rejsu
        sleep(T1 + T2);

        if (rand() % 2 == 0) { // Losowa decyzja o przerwaniu rejsu
            printf("Kapitan Portu: Wysyłam sygnał 2 (przerwanie rejsu).\n");
            signal_to_ship.type = 2;
            send_message_to_queue(message_queue_ID, &signal_to_ship, 0);
        } else {
            printf("Kapitan Portu: Rejs zakończony pomyślnie.\n");
        }
    }

    printf("Kapitan Portu: Dzienny limit rejsów osiągnięty.\n");
    delete_message_queue(message_queue_ID);

    return 0;
}