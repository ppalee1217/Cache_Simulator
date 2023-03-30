/**
 * @author ECE 3058 TAs
 */

#ifndef __CACHESIM_H
#define __CACHESIM_H

// Use these in code to make mem read/mem write/inst read related code more readable
#define MEMREAD 0
#define MEMWRITE 1
#define IFETCH 2

// Configurable parameters
#define MISS_CYCLE 10
#define MSHR_ENTRY 4
#define MAF_ENTRY 4

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
 * Struct for a cache set. Feel free to change any of this if you want. 
 */

// 1 cache set has n cache blocks, and a stack to keep track of LRU
typedef struct cache_set_t {
	int size;				// Number of blocks in this cache set
	lru_stack_t* stack;		// LRU Stack 
	cache_block_t* blocks;	// Array of cache block structs. You will need to
							// 	dynamically allocate based on number of blocks
							//	per set.
	mshr_queue_t* mshr_queue;
} cache_set_t;

void cachesim_init(int block_size, int cache_size, int ways);
void cachesim_access(addr_t physical_add, int access_type);
void cachesim_cleanup(void);
void cachesim_print_stats(void);

#endif
