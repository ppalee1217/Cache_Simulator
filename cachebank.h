#ifndef __CACHEBANK_H
#define __CACHEBANK_H

#include "cacheset.h"
#include <stdbool.h>

// Configurable parameters
#define BANK_SIZE 4

void cache_print_stats(void);
cache_bank_t* cachebank_init(int _bank_size);
void cache_init(int _bank_size, int _block_size, int _cache_size, int _ways);

#endif