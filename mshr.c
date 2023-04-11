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
  for(int i =0;i<entires;i++){
    queue[i].entries = entires;
    queue[i].mshr = init_mshr(inst_num);
  }
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

/**
 * Log the new request info issued to the DRAM
 * 
 * @param type is the operation type of the instruction - 0 for load, 1 for store, 2 for instruction read (not used).
 * @return -1 if the MAF is full, 0 if the MSHR is full, 1 if the request is not in MSHR, 2 if the request is in MSHR and MAF is not full.
 */
int mshr_queue_get_entry(mshr_queue_t* queue, unsigned long long block_addr, unsigned int type, unsigned int block_offset, unsigned int destination){
  int empty_entry = -1;
  for(int i =0 ;i<queue->entries;i++){
    if(queue[i].mshr->valid && queue[i].mshr->block_addr == block_addr){
      // This entry is already exist
      printf("MSHR %d is already exist\n", i);
      for(int j = 0; j<queue[i].mshr->maf_size;j++){
        if(!queue[i].mshr->maf[j].valid){
          // MAF is not full
          printf("MAF %d is not full, new MAF entry is added\n", j);
          queue[i].mshr->maf[j].valid = true;
          queue[i].mshr->maf[j].type = type;
          queue[i].mshr->maf_used_num++;
          // queue[i].mshr->maf[j].block_offsset = block_offset;
          // queue[i].mshr->maf[j].destination = destination;
          return 2;
        }
      }
      // If MAF is full, return -1 (to stall the pipeline)
      printf("MAF is full, stall the pipeline\n");
      return -1;
    }
  }
  for(int i=0;i<queue->entries;i++){
    // Always tend to find an empty entry has lower index
    if(!queue[i].mshr->valid){
      empty_entry = i;
      break;
    }
  }
  // printf("empty_entry = %d\n", empty_entry);
  // Check if there is any empty entry left
  if(empty_entry != -1){
    printf("MSHR %d is empty, new MSHR entry is added\n", empty_entry);
    printf("Also, MAF %d is not full, new MAF entry is added\n", 0);
    queue[empty_entry].mshr->valid = true;
    queue[empty_entry].mshr->issued = false;
    queue[empty_entry].mshr->block_addr = block_addr;
    printf("Block address of queue %d is %llx\n", empty_entry, queue[empty_entry].mshr->block_addr);
    queue[empty_entry].mshr->maf[0].valid = true;
    queue[empty_entry].mshr->maf[0].type = type;
    queue[empty_entry].mshr->maf_used_num++;
    // queue[empty_entry].mshr->maf[0].block_offsset = block_offset;
    // queue[empty_entry].mshr->maf[0].destination = destination;
    return 1;
  }
  // If there is no empty entry left, return false (to stall the pipeline)
  return 0;
}

int mshr_queue_check_isssue(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    if(queue[i].mshr->valid && !queue[i].mshr->issued){
      queue[i].mshr->issued = true;
      printf("MSHR %d issued.\n", i);
      return i;
    }
  }
  return -1;
}

void mshr_queue_check_data_returned(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    if(queue[i].mshr->valid && queue[i].mshr->issued){
      if(queue[i].mshr->counter == MISS_CYCLE){
        printf("MSHR %d data returned.\n", i);
        queue[i].mshr->counter = 0;
        queue[i].mshr->data_returned = true;
      }
    }
  }
}

bool mshr_queue_check_specific_data_returned(mshr_queue_t* queue, unsigned long long block_addr){
  for(int i =0 ;i<queue->entries;i++){
    if(queue[i].mshr->valid && queue[i].mshr->block_addr == block_addr){
      if(queue[i].mshr->data_returned)
        return true;
      else
        return false;
    }
  }
  return false;
}

void mshr_queue_counter_add(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    // printf("Queue %d valid= %d, issued =%d\n",i,queue[i].mshr->valid,queue[i].mshr->issued);
    if(queue[i].mshr->valid && queue[i].mshr->issued){
      queue[i].mshr->counter++;
      // printf("MSHR %d counter: %d\n", i, queue[i].mshr->counter);
    }
  }
}

int mshr_queue_clear_inst(mshr_queue_t* queue, int entries){
  //for(int i =0 ;i<queue->entries;i++){
  if(queue[entries].mshr->valid && queue[entries].mshr->data_returned){
    for(int j = 0; j<queue[entries].mshr->maf_size;j++){
      if(queue[entries].mshr->maf[j].valid){
        // keep looping to clear maf inst
        queue[entries].mshr->maf[j].valid = false;
        printf("Clearing MAF Queue %d\n",j);
        queue[entries].mshr->maf_used_num--;
        if(queue[entries].mshr->maf_used_num==0){
          queue[entries].mshr->valid = false;
          queue[entries].mshr->issued = false;
          queue[entries].mshr->data_returned = false;
        }
        return queue[entries].mshr->maf[j].type;
      }
    }
  }
  return -1;
  //}
}

void mshr_queue_cleanup(mshr_queue_t* queue) {
  //  TODO: Write any code if you need to do additional heap allocation
  //  cleanup
  for(int i = 0; i<queue->entries;i++){
    free(queue[i].mshr->maf);
    free(queue[i].mshr);
  }
  free(queue);
}