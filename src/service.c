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

#include "../include/client.h"
#include "../include/darray.h"
#include "../include/utils.h"
#include "../include/service.h"


void *servicer(void *args) {
    ServicerArgs *s_args = (ServicerArgs*) args;
    ClientsQueue *clients = s_args->clients;
    pid_t analyst_pid = s_args->analyst_pid;
    int *reception_done = s_args->reception_thread_done;
    
    int cont_clients = 0, fd = -1;
    int clients_satisfied = 0;
    pid_t buff_lng[N_WAKE_ANALYST];

    clock_t t_begin, t_end;
    ServiceReturnValues *ret = malloc(sizeof(ServiceReturnValues));

    t_begin = clock();
    
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
        waitpid(client->pid, NULL, WUNTRACED);
        
        time_t current_time = time(NULL);
        double waiting_time = difftime(current_time, client->t_coming);

        //pthread_cond_signal(&clients->not_full);
        pthread_mutex_lock(&clients->mutex);
        
        buff_lng[ret->clients % N_WAKE_ANALYST] = client->pid;
        ++cont_clients;

        // Contabiliza a satistafação do cliente
        if (waiting_time <= client->priority) {
            ++clients_satisfied;
        }
        pthread_mutex_unlock(&clients->mutex);

        free(client); //

        // Acordar Analista
        if (ret->clients % N_WAKE_ANALYST == 0) {
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

            sem_post(sem_block);
            kill(s_args->analyst_pid, SIGCONT);
        }

        if (darray_size(clients->data) == 0 && *s_args->reception_thread_done) {
            if (ret->clients % 10 > 0) {
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
    fclose(fopen("demanda.txt" ,"w"));

    Client *client;

    for (int i = 0; i < n_clients; i++) {
        if ((client = create_client(x_time, queue))) {
            pthread_mutex_lock(&queue->mutex);
            
            //ordered_insert(client, queue->data);
            darray_push_back(queue->data, client);

            pthread_mutex_unlock(&queue->mutex);
            pthread_cond_signal(&queue->not_empty);
        }
    }

    if (n_clients == 0) {
        while (!((ReceptionArgs*)args)->stop) {
            client = create_client(x_time, queue);
            pthread_mutex_lock(&queue->mutex);
            //ordered_insert(client, queue->data);
            darray_push_back(queue->data, client);
            /*
            while (queue->data->curr_size == 100) {
                pthread_cond_wait(&queue->not_full, &queue->mutex);
            }*/
            pthread_mutex_unlock(&queue->mutex);
        }
    }

    sem_close(sem1);
    sem_close(sem2);
    *((ReceptionArgs*) args)->reception_thread_done = 1;

    return NULL;
}

