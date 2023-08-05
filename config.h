#ifndef CONFIG_H
#define CONFIG_H

// typedef
typedef unsigned long long addr_t;
// Mapping mode
// void setAddrMappingMode(int mode){
//   #define ADDRESSING_MODE mode
// }
#define ADDRESSING_MODE 1
// Configurable parameters
#define MISS_CYCLE 20
#define REQ_QUEUE_SIZE 4
#define MSHR_ENTRY 8
#define MAF_ENTRY 4
#define BANK_SIZE 4
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
int getTotalCycle();
int getBankId();

#endif