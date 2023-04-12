#ifndef __CACHEBANK_H
#define __CACHEBANK_H

#include "cacheset.h"
#include <stdbool.h>

// Configurable parameters
#define BANK_SIZE 4
#define REQ_QUEUE_SIZE 4

cache_bank_t* cachebank_init(int _bank_size);
request_queue_t* req_queue_init(int _queue_size);
void cache_init(int _bank_size, int _block_size, int _cache_size, int _ways);
void cache_print_stats(void);
void load_request(request_queue_t* request_queue, addr_t addr, int type);
bool req_queue_full(request_queue_t* request_queue);
bool req_queue_empty(request_queue_t* request_queue);
void req_send_to_set(request_queue_t* request_queue, cache_bank_t* cache_bank, int choose);
void req_queue_cleanup(request_queue_t* request_queue);
void req_queue_forward(request_queue_t* request_queue);

#endif