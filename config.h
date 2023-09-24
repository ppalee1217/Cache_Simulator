#ifndef _CONFIG_H
#define _CONFIG_H

#define ADDRESSING_MODE 0

// Configurable parameters
#define MISS_CYCLE 4000
#define REQ_QUEUE_SIZE 8
#define MSHR_ENTRY 4        // 0 if no MSHR
#define MAF_ENTRY 8
#define BANK_SIZE 8
#define WAYS 2
#define BLOCK_SIZE 32       // 32 bytes
#define CACHE_SIZE 65536    // 64KB

// Use these in code to make mem read/mem write/inst read related code more readable
#define MEMWRITE 0
#define MEMREAD 1
#define IFETCH 2

// For IPC & NOC
//* Needs to be changed if config of noxim is changed
#define DEFAULT_QUEUE_CAPACITY 40000
#define PE_NUM 4
#define NIC_NUM 2
#define FIN_TENSOR_NUM 4    // Number of tensors that are finish tensors of graph

// Data Link Layer setting
#define MAX_PACKET_FLITNUM 16
#define MAX_FLIT_WORDSIZE 8
#endif