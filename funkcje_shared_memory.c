#include "funkcje_header.h"

int initialize_shared_memory(const char *path, int identifier, size_t size, int flags) 
{
    key_t key = ftok(path, identifier);
    if (key == -1) 
    {
        perror("Ftok error");
        exit(1);
    }

    int shmid = shmget(key, size, flags);
    if (shmid == -1) 
    {
        perror("Shmget error");
        exit(1);
    }

    return shmid;
}

void *attach_shared_memory(int shmid, const void *shmaddr, int shmflg) 
{
    void *addr = shmat(shmid, shmaddr, shmflg);
    if (addr == (void *)-1) 
    {
        perror("Shmat error");
        exit(1);
    }

    return addr;
}

void detach_shared_memory(void *shmaddr, int shmid) 
{
    if (shmdt(shmaddr) == -1) 
    {
        perror("Shmdt error");
        exit(1);
    }
}

void delete_shared_memory(int shmid) 
{
    if (shmctl(shmid, IPC_RMID, NULL) == -1) 
    {
        perror("Shmctl error");
        exit(1);
    }
}

void clear_existing_shared_memory(const char *path, int identifier) 
{
    key_t key = ftok(path, identifier);
    if (key == -1) 
    {
        perror("Błąd ftok w clear_existing_shared_memory");
        return;
    }

    int shmid = shmget(key, 0, 0666);
    if (shmid != -1) 
    {
        printf("Usuwanie istniejącej pamięci współdzielonej o ID: %d\n", shmid);
        if (shmctl(shmid, IPC_RMID, NULL) == -1) 
        {
            perror("Błąd podczas usuwania pamięci współdzielonej");
            exit(1);
        }
    }
}

