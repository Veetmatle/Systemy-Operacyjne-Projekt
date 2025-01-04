#include "funkcje_header.h"

#define MAX_PASSENGERS 10
#define BRIDGE_CAPACITY 5
#define T1 10 // Czas do możliwego odpłynięcia przed czasem
#define T2 20 // Czas trwania rejsu

int main() {
    int message_queue_ID = initialize_message_queue(".", 'q', 0666 | IPC_CREAT);
    int shared_memory_ID = initialize_shared_memory(".", 's', (MAX_PASSENGERS + 2) * sizeof(int), 0666 | IPC_CREAT);
    int *shared_memory = (int *)attach_shared_memory(shared_memory_ID, NULL, 0);

    int semid = initialize_semaphores(ftok(".", 'm'), 2);
    semaphore_signal(semid, 0, 0); // Semafor mostku
    semaphore_signal(semid, 1, 0); // Semafor pasażerów

    struct message signal_msg, passenger_exit_msg;

    while (1) {
        // Oczekiwanie na sygnał od KapitanaPortu
        printf("Kapitan Statku: Oczekiwanie na sygnał od KapitanaPortu...\n");
        receive_message_from_queue(message_queue_ID, &signal_msg, 0, 0);

        if (signal_msg.type == 2) {
            printf("Kapitan Statku: Otrzymano sygnał 2. Rejs przerwany, pasażerowie opuszczają statek.\n");
        } else if (signal_msg.type == 1) {
            printf("Kapitan Statku: Rozpoczynam rejs.\n");
            sleep(T1);
        }

        // Zablokowanie mostku
        semaphore_wait(semid, 0, 0);
        printf("Kapitan Statku: Mostek zablokowany. Rozpoczynam wyprowadzanie pasażerów.\n");

        // Wysyłanie sygnałów do pasażerów w kolejności LIFO
        for (int i = shared_memory[1] - 1; i >= 0; i--) {
            passenger_exit_msg.type = shared_memory[i + 2]; // PID pasażera jako typ komunikatu
            send_message_to_queue(message_queue_ID, &passenger_exit_msg, 0);
            printf("Kapitan Statku: Wysłałem sygnał wyjścia do pasażera PID %d.\n", shared_memory[i + 2]);

            // Oczekiwanie na potwierdzenie od pasażera
            receive_message_from_queue(message_queue_ID, &signal_msg, passenger_exit_msg.type, 0);
            printf("Kapitan Statku: Pasażer PID %d opuścił mostek.\n", shared_memory[i + 2]);
        }

        // Reset pamięci współdzielonej
        shared_memory[1] = 0; // Liczba pasażerów na statku
        semaphore_signal(semid, 0, 0); // Odblokowanie mostku
        printf("Kapitan Statku: Mostek odblokowany. Statek gotowy na załadunek.\n");
    }

    detach_shared_memory(shared_memory, shared_memory_ID);
    delete_shared_memory(shared_memory_ID);
    delete_message_queue(message_queue_ID);
    destroy_semaphores(semid);

    return 0;
}
