#pragma once

#include <stdlib.h>
#include <stdio.h>

#include "client.h"

#include "darray.h"

#include "utils.h"

typedef struct {
    int n_clients, x_time;
    ClientsQueue *clients;
} ReceptionArgs;

typedef struct {
    pid_t analyst_pid;
    ClientsQueue *clients;
} ServicerArgs;

void *servicer(void *args);

void *reception(void *args);

