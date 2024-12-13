#include "../include/utils.h"
#include <stdio.h>
#include <sys/wait.h>
#include <sys/resource.h>

void dealoc_client(void *client) {}

int ordered_insert(Client *client, Darray *clients_arr) {
    if (darray_size(clients_arr) == clients_arr->curr_size) {
        if (darray_resize (clients_arr))    return 1;
    }

    int i = clients_arr->last_data - 1;

    while (i >= 0) {
        Client *curr = clients_arr->data[i];
        if (curr->priority < client->priority) {
            clients_arr->data[i + 1] = clients_arr->data[i];
        }
        --i;
    }

    clients_arr->data[i + 1] = client;
    ++clients_arr->last_data;
    return 0;
}

void print_queue(Darray *clients_arr) {
    Client *c;
    for (int i = clients_arr->first_data; i < clients_arr->last_data; ++i) {
        c = clients_arr->data[i];
        printf("%li\n", c->t_coming);
    }
}

pid_t invoke_analyst() {
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = {"./analyst", NULL};
        execv(argv[0], argv);
        perror("analyst");
        exit(EXIT_FAILURE);
    }
    else {
        if (waitpid(pid, NULL, WUNTRACED) == -1) {
            exit(EXIT_FAILURE);
        }
        return pid;
    }
}

void print_resource_statistics() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    double user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double system_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    
    printf("Tempo executando em user mode: %.6f s\n", user_time);
    printf("Tempo executando em kernel mode: %.6f s\n", system_time);
    printf("Block output operations: %ld times\n", usage.ru_oublock);
    printf("Voluntary context switches: %ld times\n", usage.ru_nvcsw);
    printf("Involuntary context switches: %ld times\n", usage.ru_nivcsw);
    /*
    struct rusage usage_children;
    getrusage(RUSAGE_CHILDREN, &usage_children);

    double children_user_time = usage_children.ru_utime.tv_sec + usage_children.ru_utime.tv_usec / 1e6;
    double children_system_time = usage_children.ru_stime.tv_sec + usage_children.ru_stime.tv_usec / 1e6;

    printf("Uso agregado dos processos filhos:\n");
    printf("Tempo executando em user mode: %.6f s\n", children_user_time);
    printf("Tempo executando em kernel mode: %.6f s\n", children_user_time);
    printf("Block output operations: %ld times\n", usage_children.ru_oublock);
    */
}

