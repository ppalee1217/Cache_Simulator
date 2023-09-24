#ifndef _DATASTRUCT_H
#define _DATASTRUCT_H
#include <stdbool.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// typedef
typedef unsigned long long addr_t;		// Data type to hold addresses
typedef unsigned long long counter_t; // Data type to hold cache statistic variables

typedef struct tensorDependcy {
    int tensor_id;
    int packet_count;
    bool return_flag;
} tensorDependcy;


typedef struct Packet{
    int src_id;
    int dst_id;
    int packet_size;    // Flit number of this packet
    int packet_num; // Info Cache/PE that how many packets will be issued for this request(tensor)
    uint32_t packet_id;      // The packet ID
    int tensor_id;
    int* req_type;      // 1: read, 0: write
    int* flit_word_num;
    uint64_t* addr;
    int** data;
} Packet;

typedef struct traffic_t
{
    addr_t addr;    // Address to be accessed
    int* data;      // Data to be written/read
    int req_type;   // Request type (0: read, 1: write)
    int src_id;     // Source PE index
    int dst_id;     // Destination PE index (Cache bank index)
    int packet_size;    // Flit number of this packet
    int sequence_no;    // Sequence number of this flit
    uint32_t packet_id;  // The packet ID
    int tensor_id;
    int packet_num;
    int flit_word_num;
    //* For CacheSim processing
    bool valid;     // If traffic is valid
    bool working;   // If traffic is in working process by Cache Simulator
    bool finished;  // If this traffic is finished by Cache Simulator, and can be sent back to Noxim
    bool tail;      // If traffic is tail flit of a packet
} traffic_t;

typedef struct Queue{
    traffic_t* trafficTable;
    uint32_t front;
    uint32_t rear;
    uint32_t capacity;
    uint32_t size;
    int unsent_req;
} Queue;

/**
 * Struct for a request queue.
 */
typedef struct request_queue_t
{
	addr_t *request_addr;
	unsigned int *tag;
	int *request_type;
	int *req_number_on_trace;
	int queue_size;
	int req_num;
    traffic_t** traffic;
} request_queue_t;

/**
 * struct to hold a cache set's LRU stack. Note that this stack is currently intended
 * to "store the indices of cache blocks" in a cache set in an LRU order. Please make
 * sure you understand that this is NOT written to store whole cache blocks. If you
 * want to do the latter, you will have to change the LRU interface defined in this
 * file.
 */
typedef struct lru_stack_t
{
    int size; // Corresponds to the associativity
    int *priority;    // Change according to update freq
                      // If index not match, add everytime cache set update
                      // the biggest number should be replaced if stack full
} lru_stack_t;

/**
 * Struct for a cache block. Feel free to change any of this if you want.
 * One cache block has a tag, valid bit, and dirty bit (& data in hardware implementation)
 */
typedef struct cache_block_t
{
	int tag;
	bool valid;
	bool dirty;
} cache_block_t;

/**
 * Struct for a cache set.
 * One cache set has N-cache line, and a stack to keep track of LRU
 */
typedef struct cache_set_t
{
	int size;				// Number of blocks in this cache set
	lru_stack_t *stack;		// LRU Stack
	cache_block_t *blocks;  // Array of cache block structs. You will need to
                            // 	dynamically allocate based on number of blocks
                            //	per set.
} cache_set_t;

typedef struct maf_t
{
    bool valid;
    // Below is not important for this project
    unsigned int type;
    int req_number_on_trace;
    // unsigned int block_offsset;
    // unsigned int destination;
    traffic_t* traffic; //* To trace traffic of noxim
} maf_t;

typedef struct mshr_t
{
  bool valid;
  bool issued;
  bool data_returned;
  unsigned int tag;
  int maf_used_num;
  int maf_size;
  int counter;
  addr_t addr;
  maf_t *maf;
  int index;
  int last_index; // The last cleared MAF request index
} mshr_t;

typedef struct mshr_queue_t
{
  bool enable_mshr;
  int entries;
  int bank_num;
  mshr_t *mshr;
} mshr_queue_t;

/**
 * Struct for a cache bank.
 */
typedef struct cache_bank_t
{
	cache_set_t *cache_set;
	mshr_queue_t *mshr_queue;
	request_queue_t *request_queue;
	int set_num;
	bool stall;
	bool stall_type; // 1: waiting for specific address, 0: waiting for any address
	addr_t stall_addr;
	unsigned int stall_tag;
    traffic_t* stall_traffic;

	// * For non-MSHR design
	int stall_counter;
	int inst_type;

	// * For statistics per bank
	counter_t access_num;
	counter_t hit_num;
	counter_t miss_num;
	counter_t writeback_num;
	counter_t stall_MSHR_num;
	counter_t MSHR_used_times;
	counter_t MAF_used_times;
} cache_bank_t;

/**
 * Struct for a cache.
 */
typedef struct cache_t
{
	cache_bank_t *cache_bank;
	int bank_size;
} cache_t;

#endif