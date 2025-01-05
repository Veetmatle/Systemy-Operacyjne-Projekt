#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#define MSG_TYPE_PERMISSION 1
#define MSG_TYPE_RETURNED 10

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

// Funkcje do obsługi kolejki komunikatów
int initialize_message_queue(const char *path, int identifier, int flags);
void send_message_to_queue(int mesg_queue_ID, struct message *msg_ptr, int msg_flag);
void receive_message_from_queue(int mesg_queue_ID, struct message *msg_ptr, int message_type, int msg_flag);
void delete_message_queue(int mesg_queue_ID);

// Funkcje do obsługi semaforów
int initialize_semaphores(key_t key, int num_semaphores);
void semaphore_signal(int semid, int sem_num, int flags);
void semaphore_wait(int semid, int sem_num, int flags);
void destroy_semaphores(int semid);

