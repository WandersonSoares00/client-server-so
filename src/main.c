#include "../include/service.h"
#include <pthread.h>
#include <stdlib.h>

void *invoke_servicer(void *clients_queue) {
    pid_t pid = fork();
    if (pid == 0) {
        exit(0); // test
        char *argv[] = {"./analyst.c", NULL};
        execv(argv[0], argv);
        exit(EXIT_FAILURE);
    }
    else {
        ServicerArgs servicer_args = {
            .analyst_pid = pid, .clients = clients_queue
        };
       return servicer(&servicer_args);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) exit(EXIT_FAILURE);

    pthread_t reception_id, servicer_id;

    int n_clients = atoi(argv[1]);
    int time = atoi(argv[2]);
    ClientsQueue clients_queue;
    clients_queue.data = darray_init(n_clients, dealoc_client);

    pthread_mutex_init(&clients_queue.mutex, NULL);
    pthread_cond_init(&clients_queue.not_empty, NULL);
    pthread_cond_init(&clients_queue.not_full, NULL);

    ReceptionArgs reception_args = {
        .n_clients = n_clients, .x_time = time, .clients = &clients_queue
    };

    if (pthread_create(&reception_id, NULL, reception, &reception_args) != 0) {
        perror("Failed to create reception worker");
        exit(EXIT_FAILURE);
    }
    /*
    if (pthread_create(&servicer_id, NULL, invoke_servicer, &clients_queue) != 0) {
        perror("Failed to create servicer worker");
        exit(EXIT_FAILURE);
    }
    */
    pthread_join(reception_id, NULL);
    //pthread_join(servicer_id, NULL);
    
    pthread_mutex_destroy(&clients_queue.mutex);

    darray_free(clients_queue.data);
    
    return EXIT_SUCCESS;
}

