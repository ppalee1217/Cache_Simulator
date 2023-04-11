#ifndef _MSHR_H_
#define _MSHR_H_

#include <stdbool.h>

#define MISS_CYCLE 200

typedef unsigned long long addr_t;

typedef struct maf_t
{
  bool valid;
  // Below is not important for this project
  unsigned int type;
  // unsigned int block_offsset;
  // unsigned int destination;
} maf_t;

typedef struct mshr_t
{
  bool valid;
  bool issued;
  bool data_returned;
  maf_t * maf;
  int maf_used_num;
  int maf_size;
  int counter;
  addr_t block_addr;
} mshr_t;

typedef struct mshr_queue_t
{
  int entries;
  mshr_t * mshr;
} mshr_queue_t;


mshr_queue_t *init_mshr_queue(int entries, int inst_num);
// Initialize the single MSHR
mshr_t *init_mshr(int inst_num);

int mshr_queue_get_entry(mshr_queue_t* queue, unsigned long long block_addr, unsigned int type, unsigned int block_offset, unsigned int destination);
// Get the index (in order) wants to issued to the DRAM
// If there is no entry wants to issue, return -1
int mshr_queue_check_isssue(mshr_queue_t* queue);
// Getting data back from the DRAM, clear the MSHR entry
// first of all, need to clear the MAF inst
int mshr_queue_clear_inst(mshr_queue_t* queue, int entries);
// Free the memory allocated for the MSHR queue
void mshr_queue_cleanup(mshr_queue_t* queue);
// To check if the mshr request data is returned
void mshr_queue_check_data_returned(mshr_queue_t* queue);

bool mshr_queue_check_specific_data_returned(mshr_queue_t* queue, unsigned long long block_addr);

void mshr_queue_counter_add(mshr_queue_t* queue);

#endif