#ifndef _GLOBAL_PARAMETER_H_
#define _GLOBAL_PARAMETER_H_
#include "datastruct.h"
#include "config.h"

extern int addr_mode;
extern int miss_cycle;
extern int req_queue_size;
extern int MSHR_size;
extern int MAF_size;
extern int bank_num;
extern int block_size;
extern int cache_size;
extern int ways;

extern Queue* trafficTableQueue;
extern int running_mode;

#endif