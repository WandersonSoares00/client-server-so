#include "../include/utils.h"
#include <stdio.h>
#include <sys/wait.h>

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

