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
    for(int i = 0; i < capacity; i++)
        queue->trafficTable[i].data = (int*)malloc(MAX_FLIT_WORDSIZE * sizeof(int));
    return queue;
}

void enqueue(Queue* queue, traffic_t item) {
    if ((queue->rear + 1) % queue->capacity == queue->front) {
        printf("front: %d, rear: %d\n", queue->front, queue->rear);
        printf("capacity: %d, size: %d\n", queue->capacity, queue->size);
        fprintf(stderr, "Error! Queue is full.\n");
        exit(EXIT_FAILURE);
    }
    queue->unsent_req = queue->unsent_req + 1;
    queue->size = queue->size + 1;
    queue->trafficTable[queue->rear] = item;
    for(int i = 0; i < item.flit_word_num; i++){
        queue->trafficTable[queue->rear].data[i] = item.data[i];
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
}

void dequeue(traffic_t* item, Queue* queue) {
    if (queue->front == queue->rear) {
        fprintf(stderr, "Error! Queue is empty. front = %d, rear = %d\n", queue->front, queue->rear);
        exit(EXIT_FAILURE);
    }
    queue->size = queue->size - 1;
    //* Assign the item
    item->data = (int*)malloc(MAX_FLIT_WORDSIZE*sizeof(int));
    item->addr = queue->trafficTable[queue->front].addr;
    item->req_type = queue->trafficTable[queue->front].req_type;
    item->src_id = queue->trafficTable[queue->front].src_id;
    item->dst_id = queue->trafficTable[queue->front].dst_id;
    item->packet_size = queue->trafficTable[queue->front].packet_size;
    item->sequence_no = queue->trafficTable[queue->front].sequence_no;
    item->packet_id = queue->trafficTable[queue->front].packet_id;
    item->tensor_id = queue->trafficTable[queue->front].tensor_id;
    item->packet_num = queue->trafficTable[queue->front].packet_num;
    item->flit_word_num = queue->trafficTable[queue->front].flit_word_num;
    item->valid = queue->trafficTable[queue->front].valid;
    item->working = queue->trafficTable[queue->front].working;
    item->finished = queue->trafficTable[queue->front].finished;
    item->tail = queue->trafficTable[queue->front].tail;
    for(int i = 0; i < queue->trafficTable[queue->front].flit_word_num; i++){
        item->data[i] = queue->trafficTable[queue->front].data[i];
        queue->trafficTable[queue->front].data[i] = 0;
    }
    resetEntry(&queue->trafficTable[queue->front]);
    queue->front = (queue->front + 1) % queue->capacity;
}

void resetEntry(traffic_t* item){
    item->addr = 0;
    item->req_type = 0;
    item->src_id = 0;
    item->dst_id = 0;
    item->packet_size = 0;
    item->sequence_no = 0;
    item->packet_id = 0;
    item->tensor_id = 0;
    item->packet_num = 0;
    item->flit_word_num = 0;
    item->valid = false;
    item->working = false;
    item->finished = false;
    item->tail = false;
    for(int i = 0; i < MAX_FLIT_WORDSIZE; i++)
        item->data[i] = 0;
}

bool isEmpty(Queue* queue) {
    return queue->front == queue->rear;
}

bool isFull(Queue* queue) {
    return (queue->rear + 1) % queue->capacity == queue->front;
}