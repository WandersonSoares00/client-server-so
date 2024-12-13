#include "../include/service.h"
#include <pthread.h>
#include <stdlib.h>


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

    pid_t analyst_pid = invoke_analyst();
    
    int reception_thread_done = 0;

    ServicerArgs servicer_args = {
        .analyst_pid = analyst_pid, .clients = &clients_queue, .reception_thread_done = &reception_thread_done
    };

    ReceptionArgs reception_args = {
        .n_clients = n_clients, .x_time = time, .clients = &clients_queue, .reception_thread_done = &reception_thread_done
    };

    if (pthread_create(&reception_id, NULL, reception, &reception_args) != 0) {
        perror("Failed to create reception worker");
        exit(EXIT_FAILURE);
    }
       
    if (pthread_create(&servicer_id, NULL, servicer, &servicer_args) != 0) {
        perror("Failed to create servicer worker");
        exit(EXIT_FAILURE);
    }
    
    ServiceReturnValues *ret;

    pthread_join(reception_id, NULL);
    reception_thread_done = 1;
    pthread_join(servicer_id, (void**)&ret);
    
    pthread_mutex_destroy(&clients_queue.mutex);

    darray_free(clients_queue.data);

    kill(analyst_pid, SIGKILL);
    
    printf("%d clientes recebidos e %d atendidos\n", n_clients == 0 ? ret->clients : n_clients, n_clients);
    printf("Taxa de satisfação: %.2f\n",  (float) ret->clients_satisfied / ret->clients);
    printf("Tempo total de execução: %ld clocks(%f s)\n", ret->exec_time, (double) ret->exec_time / CLOCKS_PER_SEC);
    print_resource_statistics();

    return EXIT_SUCCESS;
}

