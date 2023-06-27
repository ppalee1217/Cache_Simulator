#ifndef CONFIG_H
#define CONFIG_H

// typedef
typedef unsigned long long addr_t;
// Mapping mode
#define ADDRESSING_MODE 0
// Configurable parameters
#define BANK_SIZE 4
#define REQ_QUEUE_SIZE 4
#define MSHR_ENTRY 8
#define MAF_ENTRY 4
#define MISS_CYCLE 20

#endif