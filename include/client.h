#pragma once

#include <unistd.h>
#include <time.h>

typedef struct {
    pid_t pid;
    time_t t_coming;
    int priority;
    time_t t_service;
} Client;

