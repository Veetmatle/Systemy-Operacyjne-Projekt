#include "funkcje_header.h"

int initialize_message_queue(const char *path, int identifier, int flags) 
{
    key_t key = ftok(path, identifier);
    if (key == -1) 
    {
        perror("Ftok error");
        exit(1);
    }

    int msqid = msgget(key, flags);
    if (msqid == -1) 
    {
        perror("Msgget error");
        exit(1);
    }

    return msqid;
}

void send_message_to_queue(int mesg_queue_ID, struct message *msg_ptr, int msg_flag) 
{
    while (1) 
    {
        if (msgsnd(mesg_queue_ID, (void *)msg_ptr, sizeof(msg_ptr->content), msg_flag) == -1) 
        {
            if (errno == EINTR) 
            {
                // Wywołanie przerwane przez sygnał 
                continue;
            }
            else 
            {
                perror("Msgsnd error");
                exit(1);
            }
        }
        break;
    }
}

void receive_message_from_queue(int mesg_queue_ID, struct message *msg_ptr, int message_type, int msg_flag) 
{
    while (1) 
    {
        if (msgrcv(mesg_queue_ID, (void *)msg_ptr, sizeof(msg_ptr->content), message_type, msg_flag) == -1) 
        {
            if (errno == EINTR) 
            {
                continue;
            }
            else 
            {
                perror("Msgrcv error");
                exit(1);
            }
        }
        break;
    }
}

void delete_message_queue(int mesg_queue_ID) 
{
    if (msgctl(mesg_queue_ID, IPC_RMID, NULL) == -1) 
    {
        perror("Msgctl error");
        exit(1);
    }
}

void clear_existing_message_queue(const char *path, int identifier) 
{
    key_t key = ftok(path, identifier);
    if (key == -1) 
    {
        perror("Błąd ftok w clear_existing_message_queue");
        return;
    }

    int msqid = msgget(key, 0666);
    if (msqid != -1) 
    {
        printf("Usuwanie istniejącej kolejki komunikatów o ID: %d\n", msqid);
        if (msgctl(msqid, IPC_RMID, NULL) == -1) 
        {
            perror("Błąd podczas usuwania kolejki komunikatów");
            exit(1); 
        }
    }
}

int receive_message_queue_antyprzerwanie(int msq_ID, long msgtype, struct message *msg) 
{
    while (1) 
    {
        if (msgrcv(msq_ID, msg, sizeof(msg->content), msgtype, 0) == -1) 
        {
            if (errno == EINTR) 
            {
                // Przerwane przez sygnał 
                continue;
            } else if (errno == ENOMSG) 
            {
                printf("Brak komunikatu w kolejce o typie: %ld\n", msgtype);
                continue;
            } else 
            {
                printf("Blad recieve_message: %ld\n", msgtype);
                perror("msgrcv error"); 
                return -1;
            }
        }
        return 1;
    }
}

