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
mshr_queue_t* init_mshr_queue(int bank_num, int entires, int inst_num) {
  //  Use malloc to dynamically allocate a mshr_queue_t
	mshr_queue_t* queue = (mshr_queue_t*) malloc(sizeof(mshr_queue_t));
  if(entires == 0){
    queue->enable_mshr = false;
  }
  else{
    queue->enable_mshr = true;
  }
  queue->entries = entires;
  queue->bank_num = bank_num;
  queue->mshr = init_mshr(entires ,inst_num);
  return queue;
}
/**
 * Function to initialize an single MSHR for a MSHR Queue with a given <inst_num>. This function
 * creates a MSHR. 
 * 
 * @param inst_num is the size of the MAF to initialize.
 * @return the dynamically allocated MSHR. 
 */
mshr_t* init_mshr(int entires, int inst_num){
  mshr_t* mshr = (mshr_t*) malloc(sizeof(mshr_t)*entires);
  if(entires != 0){
    for(int j=0;j<entires;j++){
      mshr[j].valid = false;
      mshr[j].issued = false;
      mshr[j].maf_size = inst_num;
      mshr[j].maf = (maf_t*) malloc(sizeof(maf_t)*inst_num);
      mshr[j].maf_used_num = 0;
      for(int i = 0; i<inst_num;i++){
        mshr[j].maf[i].valid = false;
      }
    }
  }
  return mshr;
}

/**
 * Log the new request info issued to the DRAM
 * 
 * @param type is the operation type of the instruction - 0 for load, 1 for store, 2 for instruction read (not used).
 * @return -1 if the MAF is full, 0 if the MSHR is full, 1 if the request is not in MSHR, 2 if the request is in MSHR and MAF is not full.
 */
int mshr_queue_get_entry(mshr_queue_t* queue, unsigned long long block_addr, unsigned int type, unsigned int block_offset, unsigned int destination){
  if(queue->enable_mshr){
    int empty_entry = -1;
    for(int i =0 ;i<queue->entries;i++){
      if(queue->mshr[i].valid && queue->mshr[i].block_addr == block_addr){
        // This entry is already exist
        printf("Bank %d MSHR %d is already exist\n",queue->bank_num, i);
        for(int j = 0; j<queue->mshr[i].maf_size;j++){
          if(!queue->mshr[i].maf[j].valid){
            // MAF is not full
            printf("Bank %d MAF %d is not full, new MAF entry is added\n",queue->bank_num, j);
            queue->mshr[i].maf[j].valid = true;
            queue->mshr[i].maf[j].type = type;
            queue->mshr[i].maf_used_num++;
            // queue->mshr[i].maf[j].block_offsset = block_offset;
            // queue->mshr[i].maf[j].destination = destination;
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
      if(!queue->mshr[i].valid){
        empty_entry = i;
        break;
      }
    }
    // printf("empty_entry = %d\n", empty_entry);
    // Check if there is any empty entry left
    if(empty_entry != -1){
      // printf("Bank %d MSHR %d is empty, new MSHR entry is added\n",queue->bank_num, empty_entry);
      // printf("Also, MAF %d is not full, new MAF entry is added\n", 0);
      queue->mshr[empty_entry].valid = true;
      queue->mshr[empty_entry].issued = false;
      queue->mshr[empty_entry].block_addr = block_addr;
      // printf("Block address of queue %d is %llx\n", empty_entry, queue->mshr[empty_entry].block_addr);
      queue->mshr[empty_entry].maf[0].valid = true;
      queue->mshr[empty_entry].maf[0].type = type;
      queue->mshr[empty_entry].maf_used_num++;
      // queue->mshr[empty_entry].maf[0].block_offsset = block_offset;
      // queue->mshr[empty_entry].maf[0].destination = destination;
      return 1;
    }
    // If there is no empty entry left, return false (to stall the pipeline)
    return 0;
  }
  else{

  }
}

int mshr_queue_check_isssue(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    // printf("-- Bank %d MSHR %d is valid: %d, is issued: %d\n",queue->bank_num, i, queue->mshr[i].valid, queue->mshr[i].issued);
    if(queue->mshr[i].valid && !queue->mshr[i].issued){
      queue->mshr[i].issued = true;
      printf("Bank %d MSHR %d issued.\n",queue->bank_num, i);
      return i;
    }
  }
  return -1;
}

void mshr_queue_check_data_returned(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    if(queue->mshr[i].valid && queue->mshr[i].issued){
      if(queue->mshr[i].counter == MISS_CYCLE){
        printf("Bank %d MSHR %d data returned.\n",queue->bank_num, i);
        queue->mshr[i].counter = 0;
        queue->mshr[i].data_returned = true;
      }
    }
  }
}

bool mshr_queue_check_specific_data_returned(mshr_queue_t* queue, unsigned long long block_addr){
  for(int i =0 ;i<queue->entries;i++){
    if(queue->mshr[i].valid && queue->mshr[i].block_addr == block_addr){
      if(queue->mshr[i].data_returned)
        return true;
      else
        return false;
    }
  }
  return false;
}

void mshr_queue_counter_add(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    // printf("Queue %d valid= %d, issued =%d\n",i,queue->mshr[i].valid,queue->mshr[i].issued);
    if(queue->mshr[i].valid && queue->mshr[i].issued){
      queue->mshr[i].counter++;
      // printf("MSHR %d counter: %d\n", i, queue->mshr[i].counter);
    }
  }
}

int mshr_queue_clear_inst(mshr_queue_t* queue, int entries){
  if(queue->mshr[entries].valid && queue->mshr[entries].data_returned){
    for(int j = 0; j<queue->mshr[entries].maf_size;j++){
      if(queue->mshr[entries].maf[j].valid){
        // keep looping to clear maf inst
        queue->mshr[entries].maf[j].valid = false;
        printf("Clearing Bank %d MAF Queue %d\n",queue->bank_num,j);
        queue->mshr[entries].maf_used_num--;
        if(queue->mshr[entries].maf_used_num==0){
          queue->mshr[entries].valid = false;
          queue->mshr[entries].issued = false;
          queue->mshr[entries].data_returned = false;
        }
        return queue->mshr[entries].maf[j].type;
      }
    }
  }
  return -1;
}

void mshr_queue_cleanup(mshr_queue_t* queue) {
  for(int i = 0; i<queue->entries;i++){
    free(queue->mshr[i].maf);
  }
  free(queue->mshr);
  free(queue);
}