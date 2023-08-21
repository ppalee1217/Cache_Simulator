#include "Queue.h"

Queue* createQueue(size_t capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (queue == NULL) {
        fprintf(stderr, "Memory allocation error.\n");
        exit(EXIT_FAILURE);
    }

    queue->trafficTable = (traffic_t*)malloc(capacity * sizeof(traffic_t));
    if (queue->trafficTable == NULL) {
        free(queue);
        fprintf(stderr, "Memory allocation error.\n");
        exit(EXIT_FAILURE);
    }
    queue->unsent_req = 0;
    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
    queue->capacity = capacity;

    return queue;
}

void enqueue(Queue* queue, traffic_t item) {
    if ((queue->rear + 1) % queue->capacity == queue->front) {
        printf("front: %d, rear: %d\n", queue->front, queue->rear);
        printf("capacity: %d, size: %d\n", queue->capacity, queue->size);
        fprintf(stderr, "Queue is full.\n");
        exit(EXIT_FAILURE);
    }
    queue->unsent_req = queue->unsent_req + 1;
    queue->size = queue->size + 1;
    queue->trafficTable[queue->rear] = item;
    queue->rear = (queue->rear + 1) % queue->capacity;
}

traffic_t dequeue(Queue* queue) {
    if (queue->front == queue->rear) {
        fprintf(stderr, "Queue is empty.\n");
        exit(EXIT_FAILURE);
    }
    queue->size = queue->size - 1;
    traffic_t item = queue->trafficTable[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    return item;
}

bool isEmpty(Queue* queue) {
    return queue->front == queue->rear;
}

bool isFull(Queue* queue) {
    return (queue->rear + 1) % queue->capacity == queue->front;
}