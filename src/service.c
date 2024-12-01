#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "../include/client.h"
#include "../include/darray.h"
#include "../include/utils.h"

void servicer() {

}

void create_client(int x_time, Darray *queue) {
    pid_t client_pid = fork();
    if (client_pid == 0) {
        execv("./client.c", "./client.c");
        perror("client");
        exit(EXIT_FAILURE);
    }
    else {
        Client *client = malloc(sizeof(Client));
        client->pid = client_pid;
        client->t_coming = time(NULL);
        
        int demanda_fd = open("demanda.txt", O_RDONLY);
        if (demanda_fd == -1) {
            perror("client");
            exit(EXIT_FAILURE);
        }

        int t_service;
        if (read(demanda_fd, &t_service, sizeof(t_service)) != sizeof(t_service)) {
            perror("client");
            close(demanda_fd);
            exit(EXIT_FAILURE);
        }

        close(demanda_fd);
        remove("demanda.txt");
        client->t_service = t_service;
        
        srand(time(NULL));
        if (rand() % 2) {
            client->priority = x_time;
        } else {
            client->priority = x_time / 2;
        }
        
        ordered_insert(client, queue);
    }
}

void reception(int n_clients, int x_time, Darray *queue) {
    sem_t *sem1, *sem2;
    if ((sem1 = sem_open("/sem_atend", O_CREAT)) == SEM_FAILED) {
        perror("sem_open failure");
        exit(EXIT_FAILURE);
    }
    if ((sem2 = sem_open("/sem_block", O_CREAT)) == SEM_FAILED) {
        perror("sem_open failure");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < n_clients; ++i) {
        create_client(x_time, queue);
    }

    if (n_clients == 0) {
        while (1) {
            create_client(x_time, queue);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 3)   exit(EXIT_FAILURE);
    
    int n_clients = atoi(argv[1]);
    int time =  atoi(argv[2]);
    Darray *clients_queue = darray_init(n_clients, NULL);
    
    
    darray_free(clients_queue);
    return EXIT_SUCCESS;
}

