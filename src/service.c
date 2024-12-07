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
    int *reception_done = ((ServicerArgs*) args)->reception_thread_done;
    int cont = 0;

    pthread_mutex_lock(&clients->mutex);
    
    while (darray_size(clients->data) == 0) {
        pthread_cond_wait(&clients->not_empty, &clients->mutex);
    }

    sem_t *sem_atend = sem_open("/sem_atend", O_RDWR);
    sem_t *sem_block = sem_open("/sem_block", O_RDWR);
    
    pthread_mutex_unlock(&clients->mutex);
    
    if(sem_atend == SEM_FAILED || sem_block == SEM_FAILED) {
        perror("servicer - semaphore");
        exit(EXIT_FAILURE);
    }

    while (1) {       
        pthread_mutex_lock(&clients->mutex);

        // Verficar se a fila veio vazia
        while (darray_size(clients->data) == 0) {
            pthread_cond_wait(&clients->not_empty, &clients->mutex);
        }
            
        Client *client = (Client *)darray_get_front(clients->data);        

        darray_pop_front(clients->data);
        pthread_mutex_unlock(&clients->mutex);

        // Acordar cliente
        if (kill(client->pid, SIGCONT) == -1) {
            perror("wake client");
            exit(EXIT_FAILURE);
        }
        // espera finalizar o atendimento
        sem_wait(sem_atend);
        sem_post(sem_atend);

        //pthread_cond_signal(&clients->not_full);
        ++cont;

        time_t current_time = time(NULL);
        double waiting_time = difftime(current_time, client->t_coming);

        // Retorna a satistafação do cliente
        const char *satisfaction =
            (waiting_time <= client->priority) ? "Satisfied" : "Unsatisfied";
        printf("%s\n", satisfaction);

        // Fecha o semáforo sem_block
        sem_wait(sem_block);

        // Escrever PID no LNG
        FILE *file = fopen("LNG.txt", "a");

        if (file != NULL) {
            fprintf(file, "Client: %d\n", client->pid);
            fclose(file);
        } else {
            perror("Error to open file");
            exit(EXIT_FAILURE);
        }

        sem_post(sem_block);

        free(client); //

        // Acordar Analista
        if (cont == 10) {
            kill(analyst_pid, SIGCONT);
            cont = 0;
        }

        if (darray_size(clients->data) == 0 && *reception_done) {
            if (cont > 0) {
                kill(analyst_pid, SIGCONT);
            }
            break;
        }
    }
    
    waitpid(analyst_pid, NULL, WUNTRACED);
    
    sem_close(sem_atend);
    sem_close(sem_block);

    sem_unlink("/sem_atend");
    sem_unlink("/sem_block");

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
    if ((sem1 = sem_open("/sem_atend", O_CREAT, 0666, 1)) == SEM_FAILED) {
        perror("sem_open - reception");
        exit(EXIT_FAILURE);
    }

    if ((sem2 = sem_open("/sem_block", O_CREAT, 0666, 1)) == SEM_FAILED) {
        perror("sem_open - reception");
        exit(EXIT_FAILURE);
    }

    Client *client;

    for (int i = 0; i < n_clients; i++) {
        client = create_client(x_time, queue);
        pthread_mutex_lock(&queue->mutex);
        
        //ordered_insert(client, queue->data);
        darray_push_back(queue->data, client);

        pthread_mutex_unlock(&queue->mutex);
        pthread_cond_signal(&queue->not_empty);
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

    sem_close(sem1);
    sem_close(sem2);
    *((ReceptionArgs*) args)->reception_thread_done = 1;

    return NULL;
}

