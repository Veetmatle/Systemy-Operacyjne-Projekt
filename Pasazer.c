#include "funkcje_header.h"

int main() {
    int message_queue_ID = initialize_message_queue(".", 'q', 0666 | IPC_CREAT);

    struct message exit_signal, exit_confirm;

    while (1) {
        // Oczekiwanie na sygnał wyjścia
        receive_message_from_queue(message_queue_ID, &exit_signal, getpid(), 0);
        printf("Pasażer PID %d: Otrzymałem sygnał wyjścia.\n", getpid());

        // Symulacja opuszczania mostku
        sleep(1); // Czas potrzebny na przejście mostkiem
        printf("Pasażer PID %d: Opuszczam mostek.\n", getpid());

        // Wysłanie potwierdzenia wyjścia
        exit_confirm.type = getpid();
        send_message_to_queue(message_queue_ID, &exit_confirm, 0);
        break;
    }

    return 0;
}
