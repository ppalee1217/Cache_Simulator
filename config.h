#ifndef _CONFIG_H
#define _CONFIG_H

#define ADDRESSING_MODE 0
// Configurable parameters
#define MISS_CYCLE 20
#define REQ_QUEUE_SIZE 4
#define MSHR_ENTRY 8        // 8 entries (0 if no MSHR)
#define MAF_ENTRY 4
#define BANK_SIZE 4
#define WAYS 8
#define BLOCK_SIZE 32       // 32 bytes
#define CACHE_SIZE 65536    // 64KB
// Use these in code to make mem read/mem write/inst read related code more readable
#define MEMWRITE 0
#define MEMREAD 1
#define IFETCH 2

// 
#define DEFAULT_QUEUE_CAPACITY 1000

// Mapping mode
// void setAddrMappingMode(int mode){
//   #define ADDRESSING_MODE mode
// }

// void setReqQEntryNum(int num){
//   #define REQ_QUEUE_SIZE num
// }

// void setMSHREntryNum(int num){
//   #define MSHR_ENTRY num
// }

// void setMAFEntryNum(int num){
//   #define MAF_ENTRY num
// }

// void setCacheBankNum(int num){
//   #define BANK_SIZE num
// }

// Get data from Cache
// int getTotalCycle();
// int getBankId();
#endif