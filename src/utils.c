#include "../include/utils.h"

void dealoc_client(void *client) {
    free(client);
}

int ordered_insert(Client *client, Darray *clients_arr) {
    if (darray_size(clients_arr) == clients_arr->curr_size) {
        if (darray_resize (clients_arr))    return 1;
    }

    int i = clients_arr->last_data;
    Client *curr = clients_arr->data[i];
    for (; (i >= 0 && curr && curr->priority < client->priority); i--, curr = clients_arr->data[i]) {
        clients_arr->data[i + 1] = clients_arr->data[i];
    }

    clients_arr->data[i + 1] = client;
    ++clients_arr->last_data;

    return 0;
}

