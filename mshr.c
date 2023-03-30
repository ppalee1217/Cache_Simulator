#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "mshr.h"

/**
 * Function to initialize an MSHR Queue for a cache bank with a given <entries>. This function
 * creates the MSHR Queue. 
 * 
 * @param entires is the size of the MSHR Queue to initialize.
 * @param inst_num is the size of the MAF to initialize.
 * @return the dynamically allocated queue. 
 */
mshr_queue_t* init_mshr_queue(int entires, int inst_num) {
  //  Use malloc to dynamically allocate a mshr_queue_t
	mshr_queue_t * queue = (mshr_queue_t*) malloc(sizeof(mshr_t)*entires);
  queue->entries = entires;
  mshr_t * mshr = init_mshr(inst_num);
  return queue;
}
/**
 * Function to initialize an single MSHR for a MSHR Queue with a given <inst_num>. This function
 * creates a MSHR. 
 * 
 * @param inst_num is the size of the MAF to initialize.
 * @return the dynamically allocated MSHR. 
 */
mshr_t *init_mshr(int inst_num){
  mshr_t * mshr = (mshr_t*) malloc(sizeof(mshr_t));
  mshr->valid = false;
  mshr->issued = false;
  mshr->maf_size = inst_num;
  mshr->maf = (maf_t*) malloc(sizeof(maf_t)*inst_num);
  return mshr;
}

bool mshr_queue_get_entry(mshr_queue_t* queue, unsigned long long block_addr, unsigned int type, unsigned int block_offset, unsigned int destination){
  int empty_entry = -1;
  for(int i =0 ;i<queue->entries;i++){
    // Always tend to find an empty entry has lower index
    if(!queue->mshr[i].valid && empty_entry != -1)
      empty_entry = i;
    if(queue->mshr[i].valid && queue->mshr[i].block_addr == block_addr){
      // This entry is already exist
      for(int j = 0; j<queue->mshr[i].maf_size;j++){
        if(!queue->mshr[i].maf[j].valid){
          // MAF is not full
          queue->mshr[i].maf[j].valid = true;
          // queue->mshr[i].maf[j].type = type;
          // queue->mshr[i].maf[j].block_offsset = block_offset;
          // queue->mshr[i].maf[j].destination = destination;
          return true;
        }
      }
      // If MAF is full, return false (to stall the pipeline)
      return false;
    }
  }
  // Check if there is an empty entry left
  if(empty_entry != -1){
    queue->mshr[empty_entry].valid = true;
    queue->mshr[empty_entry].issued = false;
    queue->mshr[empty_entry].block_addr = block_addr;
    queue->mshr[empty_entry].maf[0].valid = true;
    // queue->mshr[empty_entry].maf[0].type = type;
    // queue->mshr[empty_entry].maf[0].block_offsset = block_offset;
    // queue->mshr[empty_entry].maf[0].destination = destination;
    return true;
  }
  // If there is no empty entry left, return false (to stall the pipeline)
  return false;
}

int mshr_queue_check_isssue(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    if(queue->mshr[i].valid && !queue->mshr[i].issued){
      queue->mshr[i].issued = true;
      return i;
    }
  }
  return -1;
}

void mshr_queue_clear_inst(mshr_queue_t* queue, unsigned long long block_addr){
  for(int i =0 ;i<queue->entries;i++){
    if(queue->mshr[i].valid && queue->mshr[i].block_addr == block_addr){
      for(int j = 0; j<queue->mshr[i].maf_size;j++){
        if(queue->mshr[i].maf[j].valid){
          queue->mshr[i].maf[j].valid = false;
        }
      }
      queue->mshr[i].valid = false;
      queue->mshr[i].issued = false;
      queue->mshr[i].block_addr = 0;
    }
  }
}

void mshr_queue_cleanup(mshr_queue_t* queue) {
  //  TODO: Write any code if you need to do additional heap allocation
  //  cleanup
  free(queue->mshr->maf);
  free(queue->mshr);
  free(queue);
}