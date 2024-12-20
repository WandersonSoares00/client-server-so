#pragma once

#include <unistd.h>
#include <time.h>
#include "darray.h"
#include "queue.h"
#include <pthread.h>

typedef struct {
    pid_t pid;
    time_t t_coming;
    int priority;
    time_t t_service;
} Client;

typedef struct {
    Queue *q;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} ClientsQueue;
