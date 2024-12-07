#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "client.h"

#include "darray.h"

#include "utils.h"

typedef struct {
    int n_clients, x_time;
    ClientsQueue *clients;
    int *reception_thread_done;
} ReceptionArgs;

typedef struct {
    pid_t analyst_pid;
    ClientsQueue *clients;
    int *reception_thread_done;
} ServicerArgs;

void *servicer(void *args);

void *reception(void *args);

