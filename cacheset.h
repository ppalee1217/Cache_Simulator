#ifndef __CACHESET_H
#define __CACHESET_H

#include <stdbool.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include "lrustack.h"
#include "mshr.h"
#include "datastruct.h"
#include "config.h"
#include "globalParameter.h"
#include "utils.h"


cache_set_t *cacheset_init(int block_size, int cache_size, int ways);
int cacheset_access(cache_set_t *cache_set, cache_t *cache, int choose, addr_t physical_add, int access_type, unsigned int destination, counter_t *hits, counter_t *misses, counter_t *writebacks, int mode, int req_number_on_trace, traffic_t* traffic);
void cacheset_cleanup(cache_set_t *cache_set);
void cacheset_load_MSHR_data(int set_num, int choose, cache_t *cache, cache_set_t *cache_set, addr_t physical_addr, int access_type, counter_t *writebacks, counter_t *non_dirty_replaced, int mode, int req_number_on_trace);
#endif
