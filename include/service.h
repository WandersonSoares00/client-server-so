#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>

#include "client.h"

#include "darray.h"

#include "queue.h"

#include "utils.h"

typedef struct {
    int n_clients, x_time;
    ClientsQueue *clients;
    int *reception_thread_done;
    int stop;
} ReceptionArgs;

typedef struct {
    int clients;
    int clients_satisfied;
    clock_t exec_time;
    long milliseconds;
} ServiceReturnValues;

typedef struct {
    pid_t analyst_pid;
    ClientsQueue *clients;
    int *reception_thread_done;
} ServicerArgs;

void *servicer(void *args);

void *reception(void *args);

