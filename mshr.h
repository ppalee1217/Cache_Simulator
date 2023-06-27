#ifndef _MSHR_H_
#define _MSHR_H_

#include <stdbool.h>
#include "config.h"

typedef struct maf_t
{
  bool valid;
  // Below is not important for this project
  unsigned int type;
  int req_number_on_trace;
  // unsigned int block_offsset;
  // unsigned int destination;
} maf_t;

typedef struct mshr_t
{
  bool valid;
  bool issued;
  bool data_returned;
  unsigned int tag;
  int index;
  int maf_used_num;
  int maf_size;
  int counter;
  addr_t block_addr;
  maf_t * maf;
  int last_index; // The last cleared MAF request index
} mshr_t;

typedef struct mshr_queue_t
{
  bool enable_mshr;
  int entries;
  int bank_num;
  mshr_t * mshr;
} mshr_queue_t;


mshr_queue_t *init_mshr_queue(int bank_num, int entries, int inst_num);
// Initialize the single MSHR
mshr_t* init_mshr(int entires, int inst_num);

int mshr_queue_get_entry(mshr_queue_t* queue, unsigned long long block_addr, unsigned int type, unsigned int block_offset, unsigned int destination, int tag, int req_number_on_trace, int index);
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

int mshr_queue_check_exist(mshr_queue_t* queue, unsigned int tag, int index);

// bool mshr_queue_check_specific_data_returned(mshr_queue_t* queue, unsigned long long block_addr);

void mshr_queue_counter_add(mshr_queue_t* queue);

int log_maf_queue(int mshr_index, mshr_queue_t* queue, unsigned long long block_addr, unsigned int type, unsigned int block_offset, unsigned int destination, int tag);

#endif