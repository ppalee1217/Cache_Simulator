/**
 * @author ECE 3058 TAs
 */

#ifndef __CACHESET_H
#define __CACHESET_H

// Use these in code to make mem read/mem write/inst read related code more readable
#define MEMREAD 0
#define MEMWRITE 1
#define IFETCH 2

// Configurable parameters
#define MISS_CYCLE 10
#define MSHR_ENTRY 8
#define MAF_ENTRY 3

#include "lrustack.h"
#include "mshr.h"
#include <stdbool.h>

// Please DO NOT CHANGE the following two typedefs
typedef unsigned long long addr_t;		// Data type to hold addresses
typedef unsigned long long counter_t;	// Data type to hold cache statistic variables

/**
 * Struct for a cache block. Feel free to change any of this if you want. 
 */

// 1 cache block has a tag, valid bit, and dirty bit (& data in hardware implementation)
typedef struct cache_block_t {
	int tag;
	bool valid;
	bool dirty;
} cache_block_t;

/**
 * Struct for a cache set.
 */

// 1 cache set has n cache line, and a stack to keep track of LRU
typedef struct cache_set_t {
	int size;				// Number of blocks in this cache set
	lru_stack_t* stack;		// LRU Stack 
	cache_block_t* blocks;	// Array of cache block structs. You will need to
							// 	dynamically allocate based on number of blocks
							//	per set.
} cache_set_t;

/**
 * Struct for a request queue.
 */

typedef struct request_queue_t
{
  addr_t* request_addr;
	int* request_type;
  int queue_size;
	int req_num;
} request_queue_t;

/**
 * Struct for a cache bank.
 */

typedef struct cache_bank_t
{
  cache_set_t *cache_set;
  mshr_queue_t* mshr_queue;
  request_queue_t* request_queue;
	int set_num;
	bool stall;
	bool stall_type;	// 1: waiting for specific address, 0: waiting for any address
	addr_t stall_addr;
	// * For non-MSHR design
	int stall_counter;
	int inst_type;
	// ! For statistics per bank
	int hit_num;
	int miss_num;
	int writeback_num;
} cache_bank_t;

/**
 * Struct for a cache.
 */

typedef struct cache_t
{
  cache_bank_t *cache_bank;
  int bank_size;
} cache_t;

cache_set_t* cacheset_init(int block_size, int cache_size, int ways);
int cacheset_access(cache_set_t* cache_set, cache_t* cache, int choose, addr_t physical_add, int access_type, unsigned int destination, counter_t* hits, counter_t* misses, counter_t* writebacks);
void cacheset_cleanup(cache_set_t* cache_set);
void cacheset_load_MSHR_data(cache_set_t* cache_set, addr_t physical_addr, int access_type, counter_t* writebacks, counter_t* non_dirty_replaced);
int simple_log_2(int x);
#endif
