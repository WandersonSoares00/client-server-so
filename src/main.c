#include "../include/service.h"
#include <pthread.h>
#include <stdlib.h>


int main(int argc, char **argv) {
    struct options opts;

    parse_arguments(argc, argv, &opts);

    pthread_t reception_id, servicer_id, input_id;

    ClientsQueue clients_queue;
    clients_queue.data = darray_init(1, dealoc_client);
    //clients_queue.data = darray_init(opts.num_clients, dealoc_client);

    pthread_mutex_init(&clients_queue.mutex, NULL);
    pthread_cond_init(&clients_queue.not_empty, NULL);

    pid_t analyst_pid = invoke_analyst(opts.no_analyst);
    
    int reception_thread_done = 0;

    ServicerArgs servicer_args = {
        .analyst_pid = analyst_pid, .clients = &clients_queue, .reception_thread_done = &reception_thread_done
    };

    ReceptionArgs reception_args = {
        .n_clients = opts.num_clients, .x_time = opts.max_wait_time, .clients = &clients_queue, .reception_thread_done = &reception_thread_done, .stop = 0
    };

    if (pthread_create(&reception_id, NULL, reception, &reception_args) != 0) {
        perror("Failed to create reception worker");
        exit(EXIT_FAILURE);
    }
       
    if (pthread_create(&servicer_id, NULL, servicer, &servicer_args) != 0) {
        perror("Failed to create servicer worker");
        exit(EXIT_FAILURE);
    }
    
    if (opts.num_clients == 0) {
        if (pthread_create(&input_id, NULL, input_thread, &reception_args) != 0) {
            perror("Failed to create input worker");
            exit(EXIT_FAILURE);
        }
    }

    ServiceReturnValues *ret;

    pthread_join(input_id, NULL);
    pthread_join(reception_id, NULL);
    reception_thread_done = 1;
    pthread_join(servicer_id, (void**)&ret);
    
    pthread_mutex_destroy(&clients_queue.mutex);

    darray_free(clients_queue.data);

    kill(analyst_pid, SIGKILL);
    
    printf("%d clientes recebidos e %d atendidos\n", opts.num_clients == 0 ? ret->clients : opts.num_clients, ret->clients);
    printf("Taxa de satisfação: %d / %d = %.4f\n",  ret->clients_satisfied, ret->clients, (float) ret->clients_satisfied / ret->clients);
    printf("Tempo total de execução: %ld clocks(%f s)\n", ret->exec_time, (double) ret->exec_time / CLOCKS_PER_SEC);
    print_resource_statistics();

    return EXIT_SUCCESS;
}

