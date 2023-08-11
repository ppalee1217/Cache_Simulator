#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "datastruct.h"

Queue* createQueue(size_t capacity);
void enqueue(Queue* queue, traffic_t item);
traffic_t dequeue(Queue* queue);
bool isEmpty(Queue* queue);
bool isFull(Queue* queue);
#endif