#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "datastruct.h"
#include "config.h"

Queue* createQueue(size_t capacity);
void enqueue(Queue* queue, traffic_t item);
void dequeue(traffic_t* item, Queue* queue);
void resetEntry(traffic_t* item);
bool isEmpty(Queue* queue);
bool isFull(Queue* queue);
#endif