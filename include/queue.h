#pragma once

#include <stdlib.h>

typedef struct {
    void **arr;
    int front;
    int rear;
    int size;
    int size_max;
} Queue;

//#include "../include/queue.h"

Queue *queue_init(int size_max);

int queue_empty(Queue *q);

int queue_full(Queue *q);

void enqueue(Queue *q, void *data);

void *dequeue(Queue *q);

int queue_size(Queue *q);

void queue_free(Queue *q);
