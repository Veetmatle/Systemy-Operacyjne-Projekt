#include "funkcje_header.h"

int initialize_semaphores(key_t key, int num_semaphores) 
{
    int semid = semget(key, num_semaphores, IPC_CREAT | 0666);
    if (semid == -1) 
    {
        perror("Semget error");
        exit(1);
    }
    return semid;
}

void semaphore_signal(int sem_id, int sem_num, int flags) 
{
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = 1;
    sem_op.sem_flg = flags;

    while (1) 
    {
        if (semop(sem_id, &sem_op, 1) == -1) 
        {
            if (errno == EINTR) 
            {
                // Operation was interrupted by a signal, retry
                continue;
            }
            else 
            {
                perror("Semaphore Signal error");
                exit(1);
            }
        }
        break;
    }
}

void semaphore_wait(int sem_id, int sem_num, int flags) 
{
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1;
    sem_op.sem_flg = flags;

    while (1) 
    {
        if (semop(sem_id, &sem_op, 1) == -1) 
        {
            if (errno == EINTR) 
            {
                // Operation was interrupted by a signal, retry
                continue;
            }
            else 
            {
                perror("Semaphore Wait error");
                exit(1);
            }
        }
        break;
    }
}

void destroy_semaphores(int semid) 
{
    if (semctl(semid, 0, IPC_RMID) == -1) 
    {
        perror("Semctl IPC_RMID error");
        exit(1);
    }
}

void clear_existing_semaphores(const char *path, int identifier) 
{
    key_t key = ftok(path, identifier);
    if (key == -1) 
    {
        perror("Błąd ftok w clear_existing_semaphores");
        return;
    }

    int semid = semget(key, 1, 0666);
    if (semid != -1) 
    {
        printf("Usuwanie istniejących semaforów o ID: %d\n", semid);
        if (semctl(semid, 0, IPC_RMID) == -1) 
        {
            perror("Błąd podczas usuwania semaforów");
            exit(1);
        }
    }
}
