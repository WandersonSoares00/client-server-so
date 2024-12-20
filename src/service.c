#include <bits/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <aio.h>
#include <assert.h>

#include "../include/client.h"
#include "../include/utils.h"
#include "../include/service.h"


void *servicer(void *args) {
    ServicerArgs *s_args = (ServicerArgs*) args;
    ClientsQueue *clients = s_args->clients;
    pid_t analyst_pid = s_args->analyst_pid;
    
    int cont_clients = 0;
    int clients_satisfied = 0;
    pid_t buff_lng[N_WAKE_ANALYST] = {0};

    clock_t t_begin, t_end;

    ServiceReturnValues *ret = (ServiceReturnValues*) malloc(sizeof(ServiceReturnValues));
    assert (ret != NULL);

    t_begin = clock();

    pthread_mutex_lock(&clients->mutex);
    
    while (queue_size(clients->q) == 0) {
        pthread_cond_wait(&clients->not_empty, &clients->mutex);
    }

    sem_t *sem_atend = sem_open("/sem_atend", O_RDWR);
    sem_t *sem_block = sem_open("/sem_block", O_RDWR);
    
    pthread_mutex_unlock(&clients->mutex);
    
    if(sem_atend == SEM_FAILED || sem_block == SEM_FAILED) {
        perror("servicer - semaphore");
        exit(EXIT_FAILURE);
    }
    
    Client *client = NULL;

    while (1) {
        pthread_mutex_lock(&clients->mutex);

        // Verficar se a fila veio vazia
        while (queue_size(clients->q) == 0) {
            pthread_cond_wait(&clients->not_empty, &clients->mutex);
        }
            
        client = (Client *)dequeue(clients->q);

        pthread_mutex_unlock(&clients->mutex);

        pthread_cond_signal(&clients->not_full);

        // Acordar cliente
        if (kill(client->pid, SIGCONT) == -1) {
            perror("wake client");
            exit(EXIT_FAILURE);
        }
        // espera finalizar o atendimento
        waitpid(client->pid, NULL, WUNTRACED);
        
        time_t current_time = time(NULL);
        double waiting_time = difftime(current_time, client->t_coming);
 
        buff_lng[cont_clients % N_WAKE_ANALYST] = client->pid;
        ++cont_clients;

        // Contabiliza a satistafação do cliente
        if (waiting_time <= client->priority) {
            ++clients_satisfied;
        }

        free(client); //

        // Acordar Analista
        if (cont_clients % N_WAKE_ANALYST == 0) {
            // Fecha o semáforo sem_block
            if (sem_wait(sem_block) == -1) {
                perror("servicer - sem_block");
                exit(EXIT_FAILURE);
            }

            FILE *file = fopen("LNG", "ab");
            if (file != NULL) {
                fwrite(buff_lng, sizeof(pid_t), N_WAKE_ANALYST, file);
                fclose(file);
            } else {
                perror("Error to open file");
                exit(EXIT_FAILURE);
            }

            assert(sem_post(sem_block) != -1);
            kill(s_args->analyst_pid, SIGCONT);
        }

        if (queue_size(clients->q) == 0 && *s_args->reception_thread_done) {
            if (cont_clients > 0) {
                assert(sem_wait(sem_block) != -1);
                FILE *file = fopen("LNG", "ab");
                fwrite(buff_lng, sizeof(pid_t), cont_clients, file);
                fclose(file);
                assert(sem_post(sem_block) != -1);
                kill(s_args->analyst_pid, SIGCONT);
            }
            break;
        }
    }


    t_end = clock();
    waitpid(analyst_pid, NULL, WUNTRACED);
    
    sem_close(sem_atend);
    sem_close(sem_block);

    sem_unlink("/sem_atend");
    sem_unlink("/sem_block");
    
    ret->exec_time = t_end - t_begin;

    ret->clients = cont_clients;
    ret->clients_satisfied = clients_satisfied;
    
    pthread_exit(ret);
}


Client *create_client(int x_time) {
    pid_t client_pid = fork();
    if (client_pid == 0) {
        char *argv[] = {"./client", NULL};
        execv(argv[0], argv);
        perror("client");
        exit(EXIT_FAILURE);
    } else {
        Client *client = (Client*) malloc(sizeof(Client));
        assert(client != NULL);

        client->pid = client_pid;
        client->t_coming = time(NULL);

        int demanda_fd = open("demanda.txt", O_RDONLY);
        if (demanda_fd == -1) {
            perror("client");
            exit(EXIT_FAILURE);
        }

        waitpid(client_pid, NULL, WUNTRACED);
        
        if (read(demanda_fd, &client->t_service, sizeof(client->t_service)) == -1) {
            perror("client");
            close(demanda_fd);
            exit(EXIT_FAILURE);
        }
        
        close(demanda_fd);
        truncate("demanda.txt", 0);

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
    
    // cria um arquivo demanda
    FILE *file_demanda = fopen("demanda.txt" ,"w");
    assert(file_demanda != NULL);
    fclose(file_demanda);

    Client *client = NULL;

    for (int i = 0; i < n_clients; i++) {
        if ((client = create_client(x_time))) {
            pthread_mutex_lock(&queue->mutex);
            while (queue_full(queue->q))
                pthread_cond_wait(&queue->not_full, &queue->mutex);
            enqueue(queue->q, (void*) client);
            pthread_mutex_unlock(&queue->mutex);
            pthread_cond_signal(&queue->not_empty);
        }
    }

    if (n_clients == 0) {
        while (!((ReceptionArgs*)args)->stop) {
            if((client = create_client(x_time))) {
                pthread_mutex_lock(&queue->mutex);
                while (queue_full(queue->q))
                    pthread_cond_wait(&queue->not_full, &queue->mutex);
                enqueue(queue->q, (void*) client);
                while (queue_size(queue->q) >= 100) {
                    pthread_mutex_unlock(&queue->mutex);
                    pthread_cond_signal(&queue->not_empty);
                }
                pthread_mutex_unlock(&queue->mutex);
                pthread_cond_signal(&queue->not_empty);
            }
        }
    }

    assert(sem_close(sem1) != -1);
    assert(sem_close(sem2) != -1);
    *((ReceptionArgs*) args)->reception_thread_done = 1;

    return NULL;
}

