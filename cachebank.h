#ifndef __CACHEBANK_H
#define __CACHEBANK_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstdio>
#include <string.h>
#include "queue.h"
#include "globalParameter.h"
#include "cacheset.h"
#include "datastruct.h"
#include "config.h"
#include "utils.h"

extern cache_t* cache;         // Data structure for the cache
extern int cache_block_size;         // Block size
extern int cache_ways;               // cache_ways
extern int cache_num_sets;           // Number of sets
extern int cache_num_offset_bits;    // Number of block bits
extern int cache_num_index_bits;     // Number of cache_set_index bits
extern unsigned long long file_pointer_temp; // Temp file pointer for trace file
extern int req_number_on_trace;
// Statistics to keep track.
extern counter_t accesses;     // Total number of cache accesses
extern counter_t hits;         // Total number of cache hits
extern counter_t misses;       // Total number of cache misses
extern counter_t writebacks;   // Total number of writebacks
extern counter_t cycles;       // Total number of cycles
// * Added statistics
extern counter_t non_dirty_replaced; // Total number of replaced cache sets(not dirty)
extern counter_t stall_MSHR;         // Total number of stalls due to MSHR
extern counter_t stall_RequestQueue; // Total number of stalls due to Request Queue
extern counter_t MSHR_used_times;    // Total number of MSHR used times
extern counter_t MAF_used_times;     // Total number of MAF used times
extern bool enable;                 // Enable debug mode

//! For NOXIM
extern bool noxim_finish;  // Indicate if NOXIM input is finish
extern bool cache_finish;  // Indicate if cache finish all request
extern bool noc_finish[NIC_NUM]; // Indicate if noc finish all sent back request

// Running mode
void fedByTrace();
void fedByNoxim();

// Init
traffic_t* traffic_init();
cache_bank_t *cachebank_init(int _bank_size);
request_queue_t *req_queue_init(int _queue_size);
void cache_init(int _bank_size, int _block_size, int _cache_size, int _ways);

// Request Queue
void load_request(request_queue_t *request_queue, addr_t addr, unsigned int tag, int typ, traffic_t* traffic);
bool req_queue_full(request_queue_t *request_queue);
bool req_queue_empty(request_queue_t *request_queue);
void req_send_to_set(cache_t *cache, request_queue_t *request_queue, cache_bank_t *cache_bank, int choose);
void req_queue_forward(request_queue_t *request_queue);

// Function
int next_line(FILE* trace);
int checkTrafficTable(Queue* trafficTable, int nic_id);
#endif
