#pragma once

#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include "../include/client.h"
#include "../include/darray.h"

#define N_WAKE_ANALYST 10

#define N_SERVICER_THREADS() sysconf(_SC_NPROCESSORS_ONLN)

struct options {
    int num_clients;
    int max_wait_time;
    int no_analyst;
};

void parse_arguments(int argc, char *argv[], struct options *opts);

int ordered_insert(Client *client, Darray *clients_arr);

void dealoc_client(void *client);

void print_queue(Darray *clients_arr);

pid_t invoke_analyst(int lazy);

void print_resource_statistics();

void *input_thread(void* arg);

