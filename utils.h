#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <cstdlib>
#include <cstdio>
#include <string.h>
#include "globalParameter.h"
#include "datastruct.h"
#include "config.h"

#define NEW2D(H, W, TYPE) (TYPE **)new2d(H, W, sizeof(TYPE))
extern cache_t* cache;         // Data structure for the cache
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
extern int traffic_queue_num;

void* new2d(int h, int w, int size);
int simple_log_2(int x);
char* concatenateStrings(const char* str1, const char* str2);
void cache_print_stats(void);
FILE *open_trace(const char *filename);
void init_global_parameter();
void trafficTableStatus();
#endif