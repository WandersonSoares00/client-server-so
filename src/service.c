#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../include/client.h"
#include "../include/darray.h"
#include "../include/utils.h"
#include "../include/service.h"

void *servicer(void *args) {
    ClientsQueue *clients = ((ServicerArgs*) args)->clients;
    pid_t analyst_pid = ((ServicerArgs*) args)->analyst_pid;
    int cont = 0;

    while (1) {
        pthread_mutex_lock(&clients->mutex);

        // Verficar se a fila veio vazia
        while (clients->data->curr_size == 0) {
            pthread_cond_wait(&clients->not_empty, &clients->mutex);
        }

        // Esperar os semáfores sem_atend e sem_block
        sem_t *sem1 = sem_open("/sem_atend", O_RDWR);

        sem_t *sem2 = sem_open("/sem_block", O_RDWR);

        if (sem1 != SEM_FAILED) sem_wait(sem1);

        if (sem2 != SEM_FAILED) sem_wait(sem2);

        Client *client = (Client *)darray_get_front(clients->data);
        /*
        if (client == NULL) {
            sem_post(sem2);
            continue;
        }
        */
        darray_pop_front(clients->data);
        pthread_mutex_unlock(&clients->mutex);
        ++cont;
        sem_post(sem2);

        time_t current_time = time(NULL);
        double waiting_time = difftime(current_time, client->t_coming);

        // Retorna a satistafação do cliente
        const char *satisfaction =
            (waiting_time <= client->priority) ? "Satisfied" : "Unsatisfied";
        printf("%s\n", satisfaction);

        // Acordar cliente
        kill(client->pid, SIGCONT);

        // Escrever PID no LNG
        FILE *file = fopen("LNG.txt", "a");
        if (file != NULL) {
            fprintf(file, "Client: %d\n", client->pid);
            fclose(file);
        } else {
            perror("Error to open file");
            exit(EXIT_FAILURE);
        }

        // Acordar Analista
        if (cont == 10) {
            kill(analyst_pid, SIGCONT);
            cont = 0;
        }
    }
    return NULL;
}

Client *create_client(int x_time, ClientsQueue *queue) {
    pid_t client_pid = fork();
    if (client_pid == 0) {
        char *argv[] = {"./client", NULL};
        execv(argv[0], argv);
        perror("client");
        exit(EXIT_FAILURE);
    } else {
        Client *client = malloc(sizeof(Client));
        client->pid = client_pid;
        client->t_coming = time(NULL);
        
        waitpid(client_pid, NULL, WUNTRACED);

        int demanda_fd = open("demanda.txt", O_RDONLY);
        if (demanda_fd == -1) {
            perror("client");
            exit(EXIT_FAILURE);
        }

        int t_service;
        if (read(demanda_fd, &t_service, sizeof(t_service)) == -1) {
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

        return client;
    }
}

void *reception(void *args) {
    int x_time = ((ReceptionArgs*) args)->x_time;
    int n_clients = ((ReceptionArgs*) args)->n_clients;
    ClientsQueue *queue = ((ReceptionArgs*) args)->clients;
    
    sem_t *sem1, *sem2;
    if ((sem1 = sem_open("/sem_atend", O_CREAT, 0644, 1)) == SEM_FAILED) {
        perror("sem_open failure");
        exit(EXIT_FAILURE);
    }

    if ((sem2 = sem_open("/sem_block", O_CREAT, 0644, 1)) == SEM_FAILED) {
        perror("sem_open failure");
        exit(EXIT_FAILURE);
    }

    Client *client;
  

    for (int i = 0; i < n_clients; i++) {
        client = create_client(x_time, queue);
        pthread_mutex_lock(&queue->mutex);
        ordered_insert(client, queue->data);
        pthread_mutex_unlock(&queue->mutex);
    }

    if (n_clients == 0) {
        while (1) {
            client = create_client(x_time, queue);
            pthread_mutex_lock(&queue->mutex);
            ordered_insert(client, queue->data);
            while (queue->data->curr_size == 100) {
                pthread_cond_wait(&queue->not_full, &queue->mutex);
            }
            pthread_mutex_unlock(&queue->mutex);
        }
    }

    return NULL;
}

