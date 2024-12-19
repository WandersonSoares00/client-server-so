#pragma once

#include <unistd.h>
#include <time.h>
#include "darray.h"
#include <pthread.h>

typedef struct {
    pid_t pid;
    time_t t_coming;
    int priority;
    time_t t_service;
} Client;

typedef struct {
    Darray *data;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} ClientsQueue;


