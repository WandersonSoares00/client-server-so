#include "../include/queue.h"
#include <assert.h>

Queue *queue_init(int size_max) {
    Queue *q = (Queue*) malloc(sizeof(Queue));
    assert(q != NULL);
    q->arr = malloc(sizeof(void*) * size_max);
    assert(q->arr != NULL);
    q->size_max = size_max;
    q->front = q->rear = q->size = 0;
    return q;
}

inline int queue_empty(Queue *q) {
    return q->size == 0;
}

inline int queue_full(Queue *q){
    return q->size == q->size_max;
}

void enqueue(Queue *q, void *data){
    assert(!queue_full(q));
    q->arr[q->rear] = data;
    q->rear = (q->rear + 1) % q->size_max;
    ++q->size;
}

void *dequeue(Queue *q){
    assert(!queue_empty(q));
    void *x = q->arr[q->front];
    q->front = (q->front + 1) % q->size_max;
    --q->size;
    return x;
}

int queue_size(Queue *q) {
    return q->size;
}

void queue_free(Queue *q) {
    free(q->arr);
    free(q);
    q = NULL;
}
