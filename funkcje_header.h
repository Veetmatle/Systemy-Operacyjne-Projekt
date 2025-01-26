#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdarg.h>
#include <stdbool.h>

#define MSG_TYPE_PERMISSION 1 // pakowanie sie na statek
#define MSG_TYPE_RETURNED 10 // statek wrocil
#define MSG_TYPE_PORT_READY 30 // port gotowy do obslugi przez KP (losowanie decyzji) 
#define MSG_TYPE_NORMAL_DEPARTURE 50 // wiadomosc o normalnym odplywaniu od KP
#define MSG_TYPE_END_OF_OPERATION 40 // wiadomosc do kapitana portu zeby przerwal obsluge portu
#define MSG_TYPE_PORT 55 //wiadomosc do kapitana portu zeby zaczal zarzadzac portem
#define MSG_TYPE_EARLY_DEPARTURE 99 // wiadomosc o wczesniejszym odplywaniu
#define MSG_TYPE_SIGNAL_TO_CAPTAIN_YOU_CAN_LEAVE 222 // sygnał do kapitana ze moze jechac (po signalu)
#define MSG_TYPE_SIGNAL_TO_CAPTAIN_YOU_CAN_LEAVE2 221
#define MSG_TYPE_FIRST_PORT_SIGNAL 181 //sygnał do kapitana portu od kapitana statku z pidem ze moze losowac czy przerywac rejsy
#define MSG_TYPE_END_OF_PORT 111 //sygnał od kapitana portu ze koniec rejsow
#define MSG_TYPE_SECOND_PORT_SIGNAL 88 // druga wiadomosc do portu (tuz przed rejsem)

// do semaforow kapitanow
#define SEM_SHIP_DEPARTED 0
#define SEM_PORT_PROCESSED 1

// statek i kładka
#define MAX_ON_BRIDGE 5   
#define MAX_ON_SHIP 40
#define PASSENGERS 50

// kolorki
#define LIGHTBLUE "\033[94m" 
#define GREEN   "\033[32m"    
#define RED     "\033[31m"    
#define RESET   "\033[0m"     

// rzeczy do rejsow
#define T1 6000000      //?6s      // Czas między odpłynięciami
#define T2 1000000 //1s            // Czas trwania rejsu
#define R 10  

struct shared_data 
{
    int total_passengers;      // całkowita liczba pasażerów
    int passengers_left;       // liczba pozostałych pasażerów
    volatile int is_finished;  // flaga zakończenia
};

// Struktura komunikatu
struct message 
{
    long type;
    int content;
};

// Funkcje do obsługi pamięci dzielonej
int initialize_shared_memory(const char *path, int identifier, size_t size, int flags);
void *attach_shared_memory(int shmid, const void *shmaddr, int shmflg);
void detach_shared_memory(void *shmaddr, int shmid);
void delete_shared_memory(int shmid);
void clear_existing_shared_memory(const char *path, int identifier);

// Funkcje do obsługi kolejki komunikatów
int initialize_message_queue(const char *path, int identifier, int flags);
void send_message_to_queue(int mesg_queue_ID, struct message *msg_ptr, int msg_flag);
void receive_message_from_queue(int mesg_queue_ID, struct message *msg_ptr, int message_type, int msg_flag);
void delete_message_queue(int mesg_queue_ID);
void clear_existing_message_queue(const char *path, int identifier);
int receive_message_queue_antyprzerwanie(int msq_ID, long msgtype, struct message *msg);

// Funkcje do obsługi semaforów
int initialize_semaphores(key_t key, int num_semaphores);
void semaphore_signal(int semid, int sem_num, int flags);
void semaphore_wait(int semid, int sem_num, int flags);
void destroy_semaphores(int semid);
void clear_existing_semaphores(const char *path, int identifier);