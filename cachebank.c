#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "cachebank.h"

// Statistics to keep track.
counter_t accesses = 0;     // Total number of cache accesses
counter_t hits = 0;         // Total number of cache hits
counter_t misses = 0;       // Total number of cache misses
counter_t writebacks = 0;   // Total number of writebacks
counter_t cycles = 0;       // Total number of cycles
// * Added statistics
counter_t non_dirty_replaced = 0; // Total number of replaced cache sets(not dirty)
counter_t stall_MSHR = 0;         // Total number of stalls due to MSHR
counter_t stall_RequestQueue = 0; // Total number of stalls due to Request Queue
counter_t MSHR_used_times = 0;    // Total number of MSHR used times
counter_t MAF_used_times = 0;     // Total number of MAF used times

cache_t* cache;         // Data structure for the cache
int cache_block_size;         // Block size
int cache_size;         // Cache size
int cache_ways;               // cache_ways
int cache_num_sets;           // Number of sets
int cache_num_offset_bits;    // Number of block bits
int cache_num_index_bits;     // Number of cache_set_index bits
unsigned long long file_pointer_temp; // Temp file pointer for trace file

request_queue_t* req_queue_init(int _queue_size){
  request_queue_t* request_queue = (request_queue_t*) malloc(sizeof(request_queue_t));
  request_queue->request_addr = (addr_t*) malloc(_queue_size*sizeof(addr_t));
  request_queue->request_type = (int*) malloc(_queue_size*sizeof(int));
  request_queue->queue_size = _queue_size;
  return request_queue;
}

/**
 * Function to intialize cache simulator with the given cache parameters. 
 * Note that we will only input valid parameters and all the inputs will always 
 * be a power of 2.
 * 
 * @param bank_size is the number of cache banks
 * @param _block_size is the block size in bytes
 * @param _cache_size is the cache bank size in bytes
 * @param _ways is the associativity
 */
cache_bank_t* cachebank_init(int _bank_size) {
  cache_bank_t* cache_bank = (cache_bank_t*) malloc(_bank_size*sizeof(cache_bank_t));
  // printf("cache size = %d\n",cache_size);
  // printf("cache set num = %d\n",cache_num_sets);
  for(int i=0;i<_bank_size;i++) {
    cache_bank[i].set_num = cache_num_sets/4;
    cache_bank[i].cache_set = cacheset_init(cache_block_size, cache_size/_bank_size, cache_ways);
   // printf("cache bank %d set num = %d\n",i,cache_num_sets/4);
   // printf("cache bank %d cache size = %d\n",i,cache_size/_bank_size);
    cache_bank[i].mshr_queue = init_mshr_queue(i ,MSHR_ENTRY, MAF_ENTRY);
    cache_bank[i].request_queue = req_queue_init(REQ_QUEUE_SIZE);
    cache_bank[i].stall_counter = 0;
  }
  return cache_bank;
}

void cache_init(int _bank_size, int _block_size, int _cache_size, int _ways) {
  cache_block_size = _block_size;   // In byte
  cache_size = _cache_size;   // In byte
  cache_ways = _ways;
  cache = (cache_t*) malloc(sizeof(cache_t));
  cache_num_sets = cache_size / (cache_block_size * cache_ways);
  cache->bank_size = _bank_size;
  cache->cache_bank = cachebank_init(_bank_size);
  cache_num_offset_bits = simple_log_2((cache_block_size/4));
  cache_num_index_bits = simple_log_2(cache_num_sets);
}

void load_request(request_queue_t* request_queue, addr_t addr, int type){
  request_queue->request_addr[request_queue->req_num] = addr;
  request_queue->request_type[request_queue->req_num] = type;
  request_queue->req_num++;
}

bool req_queue_full(request_queue_t* request_queue){
  if(request_queue->req_num == request_queue->queue_size){
    printf("Request queue is full\n");
    return true;
  }
  else{
    return false;
  }
}

bool req_queue_empty(request_queue_t* request_queue){
  if(request_queue->req_num == 0){
    return true;
  }
  else{
    return false;
  }
}

void req_queue_forward(request_queue_t* request_queue){
  printf("Request queue forwarding...\n");
  for(int i=0;i<request_queue->req_num-1;i++){
    request_queue->request_addr[i] = request_queue->request_addr[i+1];
    request_queue->request_type[i] = request_queue->request_type[i+1];
  }
  request_queue->req_num = request_queue->req_num - 1;
}

void req_send_to_set(cache_t* cache,request_queue_t* request_queue, cache_bank_t* cache_bank, int choose){
  // Check if this cache bank is stalling for MSHR Queue
  // if true, update the MSHR & MAF info
  // else, send the request to cache set
  printf("Bank %d Request queue num = %d\n",choose,request_queue->req_num);
  if(cache_bank[choose].stall){
    // ! Check stall status of non-MSHR design 
    if(!cache_bank[choose].mshr_queue->enable_mshr){
      cache_bank[choose].stall_counter++;
      // printf("Bank %d is stalled, counter = %d\n",choose,cache_bank[choose].stall_counter);
      // getchar();
      // * Miss data returned, load data to cache
      if(cache_bank[choose].stall_counter == MISS_CYCLE){
        cache_bank[choose].stall = false;
        cache_bank[choose].stall_counter = 0;
        cacheset_load_MSHR_data(cache_bank[choose].set_num, choose, cache, cache_bank[choose].cache_set, cache_bank[choose].stall_addr, cache_bank[choose].inst_type, &writebacks, &non_dirty_replaced, ADDRESSING_MODE);
        req_queue_forward(request_queue);
      }
    }
    if(cache_bank[choose].stall_type){
     // printf("Cache bank %d : waiting for specific data to return...\n",choose);
     // printf("Cache bank %d is waiting for addr %p to return.\n",choose,cache_bank[choose].stall_addr);
      for(int i=0;i<cache_bank[choose].mshr_queue->entries;i++){
       // printf("MSHR Queue %d address is %p => issue: %d, return: %d\n",i,cache_bank[choose].mshr_queue->mshr[i].block_addr,cache_bank[choose].mshr_queue->mshr[i].issued,cache_bank[choose].mshr_queue->mshr[i].data_returned);
      }
    }
    else{
     // printf("Cache bank %d : waiting for any data to return & clear...\n",choose);
    }
    return;
  }
  else{
    // Setting offset
    unsigned int offset = 0;
    for(int i=0;i<cache_num_offset_bits;i++){
      offset = (offset << 1) + 1;
    }
    offset = ((request_queue->request_addr[0] >> 2) & offset);
    // Check if the request queue is empty
    if(req_queue_empty(request_queue)){
     // printf("Request queue is empty, No need to send.\n");
    }
    else{
        printf("Sending request to cache bank %d\n",choose);
      int maf_result = cacheset_access(cache_bank[choose].cache_set, cache, choose, request_queue->request_addr[0], request_queue->request_type[0], 0, &hits, &misses, &writebacks, ADDRESSING_MODE);
      // Check the MSHR Queue
      if(maf_result == 3){
        // * hit
        
      }
      else if(maf_result == 1){
        //* printf("This request is not in MSHR Queue, and is logged now\n");
        // request is successfully logged and MAF is not full
        // => check if the request is issued to DRAM, and log MAF (is done by mshr_queue_get_entry)
        // * log instruction to MAF if the data is returned or not
        MSHR_used_times++;
        cache_bank[choose].MSHR_used_times++;
      }
      else if(maf_result == 2){
        //* printf("This request is in MSHR Queue, and MAF is not full yet.\n");
        // request is successfully logged and MAF is not full
        // => check if the request is issued to DRAM, and log MAF (is done by mshr_queue_get_entry)
        // * log instruction to MAF if the data is returned or not
        MAF_used_times++;
        MSHR_used_times++;
        cache_bank[choose].MSHR_used_times++;
        cache_bank[choose].MAF_used_times++;
      }
      else if(maf_result == -1){
        //* printf("This request is in MSHR Queue, but MAF is full.\n");
        // request is already in the queue, but MAF is full
        // => check if the request is issued to DRAM, and wait for this request to be done(stall => while loop)
        // * Wait for data to return, and clear 1 instruction so that this new instruction can be logged.
        // * meantime, MSHR will continue handle the returned data unexecuted instruction
        
        // ! Update request queue stall info
        cache_bank[choose].stall = true;
        cache_bank[choose].stall_type = 1;
        cache_bank[choose].stall_addr = request_queue->request_addr[0];
        if(cache_bank[choose].mshr_queue->enable_mshr){
          stall_MSHR++;
          cache_bank[choose].stall_MSHR_num++;
        }
        return;
      }
      else{
        //* printf("This request is not in MSHR Queue.\n");
        // * MSHR queue is full, wait for any data to return, and then wait for the MAF queue to be cleared
        cache_bank[choose].stall = true;
        cache_bank[choose].stall_type = 0;
        // * For non-MSHR design
        cache_bank[choose].stall_addr = request_queue->request_addr[0];
        cache_bank[choose].inst_type = request_queue->request_type[0];
        if(cache_bank[choose].mshr_queue->enable_mshr){
          stall_MSHR++;
          cache_bank[choose].stall_MSHR_num++;
        }
        return;
      }
      // log the info of returned data to cache
      // request is not in the queue
      // ! Remember to clear the request in the queue
      req_queue_forward(request_queue);
      // printf("Request queue %d num is now %d\n",choose,request_queue->req_num);
    }
  }
}

/**
 * Function to open the trace file
 * You do not need to update this function. 
 */
FILE *open_trace(const char *filename) {
    return fopen(filename, "r");
}

/**
 * Read in next line of the trace
 * 
 * @param trace is the file handler for the trace
 * @return 0 when error or EOF and 1 when successful, 2 when request queue is full.
 */
int next_line(FILE* trace) {
  if (feof(trace) || ferror(trace)) return 0;
  else {
    int t;
    unsigned long long address, instr;
    unsigned int index_mask = 0;
    unsigned int cache_set_index = 0;
    fscanf(trace, "%d %llx %llx\n", &t, &address, &instr);
    for(int i=0;i<cache_num_index_bits;i++){
      cache_set_index = (cache_set_index << 1) + 1;
      index_mask = (index_mask << 1) + 1;
    }
    cache_set_index = ((address >> (cache_num_offset_bits+2)) & index_mask);
    printf("Index = %lld\n", cache_set_index);
    // printf("cache num bits = %x\n",cache_num_offset_bits);
    // printf("cache index bits = %x\n",cache_num_index_bits);
    // printf("Cache num sets = %d\n",cache_num_sets*cache_ways);
    // printf("Address = 0x%llx\n", address);
    if(ADDRESSING_MODE == 0){
      // int shift_num = 31 - bank_mask_bit_num;
      // int choose_bank = ((address & (bank_mask << shift_num)) >> shift_num);
      int choose_bank = cache_set_index % BANK_SIZE;
      // printf("Choose bank %d\n",choose_bank);
      if(req_queue_full(cache->cache_bank[choose_bank].request_queue)){
        fseek(trace, file_pointer_temp, SEEK_SET);
        // printf("After fseek: trace pointer = %lld\n",file_pointer_temp);
        return 2;
      }
      else{
        cache->cache_bank[choose_bank].access_num++;
        printf("Bank %d access num = %lld\n",choose_bank,cache->cache_bank[choose_bank].access_num);
        load_request(cache->cache_bank[choose_bank].request_queue, address, t);
        printf("Request is load to queue %d\n",choose_bank);
      }
    }
    else if(ADDRESSING_MODE == 1){
      unsigned int address_partition = ((unsigned int)-1) / BANK_SIZE;
      int choose_bank = address / address_partition;
      // printf("Choose bank %d\n",choose_bank);
      if(req_queue_full(cache->cache_bank[choose_bank].request_queue)){
        // printf("request queue %d is full\n",choose_bank);
        fseek(trace, file_pointer_temp, SEEK_SET);
        // printf("After fseek: trace pointer = %lld\n",file_pointer_temp);
        return 2;
      }
      else{
        cache->cache_bank[choose_bank].access_num++;
        load_request(cache->cache_bank[choose_bank].request_queue, address, t);
        // printf("request is load to queue %d\n",choose_bank);
      }
    }
    else if(ADDRESSING_MODE == 2){

    }
    accesses++;
  }
  return 1;
}

/**
 * Main function. See error message for usage. 
 * 
 * @param argc number of arguments
 * @param argv Argument values
 * @returns 0 on success. 
 */
int main(int argc, char **argv) {
    FILE *input;

    if (argc != 5) {
        fprintf(stderr, "Usage:\n  %s <trace> <block size(bytes)>"
                        " <cache size(bytes)> <cache_ways>\n", argv[0]);
        return 1;
    }
    
    input = open_trace(argv[1]);
    cache_init(BANK_SIZE, atol(argv[2]), atol(argv[3]), atol(argv[4]));
    int input_status = 1;
    while (input_status != 0){
      cycles++;
      printf("=======================\n");
      printf("- Cycles: %lld\n",cycles);
      // * Load input to request queue
      for(int i =0;i<cache->bank_size;i++){
        file_pointer_temp = ftell(input);
        // printf("trace pointer = %lld\n",file_pointer_temp);
        input_status = next_line(input);
        if(input_status == 0){
          // printf("Instruction input complete.\n");
          break;
        }
        else if(input_status == 2){
          // printf("The desired request queue is full, need to handle request first.\n");
          break;
        }
      }
      if(input_status == 2)
        stall_RequestQueue++;
      // * Check the MSHR & MAF status for each bank
      printf("---------------------\n");
      for(int j=0;j<cache->bank_size;j++){
        // ! Only need to check MSHR status when MSHR enabled
        if(cache->cache_bank[j].mshr_queue->enable_mshr){
          // Update the status of MSHR queue of each bank
          mshr_queue_check_isssue(cache->cache_bank[j].mshr_queue);
          mshr_queue_counter_add(cache->cache_bank[j].mshr_queue);
          mshr_queue_check_data_returned(cache->cache_bank[j].mshr_queue);
          // Check if data in each MSHR entries is returned
          for(int i=0; i<cache->cache_bank[j].mshr_queue->entries;i++){
            if(cache->cache_bank[j].mshr_queue->mshr[i].data_returned && cache->cache_bank[j].mshr_queue->mshr[i].valid){
              printf("Clearing instruction of Bank %d MSHR Queue %d\n",j,i);
              // ! Check if the return addr is satlling the cache bank
              if(cache->cache_bank[j].stall){
                if(cache->cache_bank[j].stall_type){
                  if(cache->cache_bank[j].stall_addr == cache->cache_bank[j].mshr_queue->mshr[i].block_addr){
                    // printf("Specific data in cache bank %d MSHR Queue %d is returned, clear stall\n",j,i);
                    cache->cache_bank[j].stall = false;
                    cache->cache_bank[j].stall_type = 0;
                    cache->cache_bank[j].stall_addr = 0;
                  }
                }
                else{
                  // printf("Any data in cache bank %d MSHR Queue %d is returned, clear stall\n",j,i);
                  cache->cache_bank[j].stall = false;
                }
              }
              // Load returned data to cache set
              cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, ADDRESSING_MODE);
            }
          }
        }
        // * When request queue is not empty, send request to cache bank
        if(!req_queue_empty(cache->cache_bank[j].request_queue)){
          // getchar();
          req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
        }
        // * Deal with unresolved MAF
      }
      // // getchar();
    }
    // ! Execution not done yet => request queue need to be cleared
    printf("Instruction input complete. Clearing Request Queue & MSHR Queue...\n");
    printf("Cycle: %lld\n", cycles);
    printf("=====================\n");
    //* Print the state of queue in each bank
    for(int j = 0;j<cache->bank_size;j++){
      printf("Bank %d:\n",j);
      for(int i = 0;i<cache->cache_bank[j].mshr_queue->entries;i++){
          if(cache->cache_bank[j].mshr_queue->mshr[i].valid){
            printf("Bank %d MSHR Queue %d is not clear\n",j,i);
          }
          else{
            printf("Bank %d MSHR Queue %d is clear\n",j,i);
          }
      }
      if(cache->cache_bank[j].request_queue->req_num != 0){
        printf("Bank %d Request Queue is not clear\n",j);
      }
      else{
        printf("Bank %d Request Queue is clear\n",j);
      }
    }
    // * MSHR enabled => need to check if MSHR is cleared(stalled) & request queue is cleared
    if(cache->cache_bank[0].mshr_queue->enable_mshr){
      bool MSHR_not_clear = false;
      bool req_queue_not_clear = false;
      //! First check after input instruction is done
      //* (First) Check if request queue is clear
      for(int j =0;j<cache->bank_size;j++){
        if(!req_queue_empty(cache->cache_bank[j].request_queue)){
          req_queue_not_clear = true;
          break;
        }
      }
      //* (First) Check if MSHR queue is clear
      for(int j = 0;j<cache->bank_size;j++){
        for(int i=0;i<cache->cache_bank[j].mshr_queue->entries;i++){
          if(cache->cache_bank[j].mshr_queue->mshr[i].valid)
            MSHR_not_clear = true;
        }
      }
      //* Loop through until MSHR and request queue is clear
      while(MSHR_not_clear || req_queue_not_clear){
        //* Update cycle number
        cycles++;
        printf("- Cycles: %lld\n",cycles);
        MSHR_not_clear = false;
        req_queue_not_clear = false;
        //* Update MSHR queue status
        for(int j = 0;j<cache->bank_size; j++){
          mshr_queue_check_isssue(cache->cache_bank[j].mshr_queue);
          mshr_queue_counter_add(cache->cache_bank[j].mshr_queue);
          mshr_queue_check_data_returned(cache->cache_bank[j].mshr_queue);
        }
        //* Check MSHR queue status
        for(int j = 0;j<cache->bank_size;j++){
          for(int i=0;i<cache->cache_bank[j].mshr_queue->entries;i++){
            //* Clear the MSHR
            if(cache->cache_bank[j].mshr_queue->mshr[i].valid && cache->cache_bank[j].mshr_queue->mshr[i].data_returned){
              printf("Clearing instruction of Bank %d MSHR Queue %d\n",j,i);
              // ! Check if return addr is satlling the cache bank
              if(cache->cache_bank[j].stall){
                if(cache->cache_bank[j].stall_type){
                  if(cache->cache_bank[j].stall_addr == cache->cache_bank[j].mshr_queue->mshr[i].block_addr){
                    printf("Specific data in cache bank %d MSHR Queue %d is returned, clear stall\n",j,i);
                    cache->cache_bank[j].stall = false;
                    cache->cache_bank[j].stall_type = 0;
                    cache->cache_bank[j].stall_addr = 0;
                  }
                }
                else{
                  printf("Any data in cache bank %d MSHR Queue %d is returned, clear stall\n",j,i);
                  cache->cache_bank[j].stall = false;
                }
              }
              //* Handle the data returned MSHR queue inst.
              cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, ADDRESSING_MODE);
            }
          }
          //* If request queue is not empty, send request to cache bank
          if(!req_queue_empty(cache->cache_bank[j].request_queue))
            req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
        }
        //* Check if MSHR queue not clear yet
        for(int j = 0;j<cache->bank_size;j++)
          for(int i=0;i<cache->cache_bank[j].mshr_queue->entries;i++)
            if(cache->cache_bank[j].mshr_queue->mshr[i].valid)
              MSHR_not_clear = true;
        //* Check if Request queue not clear yet
        for(int j =0;j<cache->bank_size;j++){
          if(!req_queue_empty(cache->cache_bank[j].request_queue)){
            req_queue_not_clear = true;
            break;
          }
        }
        printf("=====================\n");
      }
    }
    // * MSHR is not enabled, check if request queue is cleared & (cache bank is stalled)
    else{
      // Check if cache bank is stalled
      bool check_stall = false;
      bool req_queue_not_clear = false;
      for(int j =0;j<cache->bank_size;j++){
        if(!req_queue_empty(cache->cache_bank[j].request_queue)){
          req_queue_not_clear = true;
          break;
        }
      }
      for(int j = 0;j<cache->bank_size;j++){
        if(cache->cache_bank[j].stall){
          check_stall = true;
          break;
        }
      }
      while(req_queue_not_clear || check_stall){
        check_stall = false;
        req_queue_not_clear = false;
        printf("non-MSHR design is not cleared yet, wait for bank and request queue to be done.\n");
        cycles++;
        for(int j = 0;j<cache->bank_size;j++){
          if(!req_queue_empty(cache->cache_bank[j].request_queue)){
            printf("Bank %d request still got %d requests.\n",j ,cache->cache_bank[j].request_queue->req_num);
            if(cache->cache_bank[j].stall)
              printf("Bank %d is also stalled.\n", j);
            req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
          }
          else if(cache->cache_bank[j].stall){
            printf("Bank %d is now stalled.\n", j);
            req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
          }
        }
        for(int j =0;j<cache->bank_size;j++){
        if(!req_queue_empty(cache->cache_bank[j].request_queue)){
          req_queue_not_clear = true;
          break;
        }
        }
        for(int j = 0;j<cache->bank_size;j++){
          if(cache->cache_bank[j].stall){
            check_stall = true;
            break;
          }
        }
      }
    }
    cache_print_stats();
    // Cleanup
    for(int i=0;i<cache->bank_size;i++){
      cacheset_cleanup(cache->cache_bank[i].cache_set);
      mshr_queue_cleanup(cache->cache_bank[i].mshr_queue);
    }
    free(cache->cache_bank);
    free(cache);
    fclose(input);
    return 0;
}

/**
 * Function to printf cache statistics.
 */
void cache_print_stats() {
  printf("With %d cache_ways associativity, %d sets, and %d bytes per cache set:\n", cache_ways, cache_num_sets, cache_block_size);
  printf("Cycles          : %llu\n", cycles);
  printf("Accesses        : %llu\n", accesses);
  printf("Hits            : %llu\n", hits); 
  printf("Misses          : %llu\n", misses); 
  printf("Writebacks      : %llu\n", writebacks); 
  printf("Hit rate        : %.3f%%\n", (double)hits/accesses*100);
  printf("Miss rate       : %.3f%%\n", (double)misses/accesses*100);
  //  printf("Writeback rate  : %.3f%%\n", (double)writebacks/accesses*100);
  printf("MSHR used times : %llu\n", MSHR_used_times);
  printf("MSHR used rate  : %.3f%%\n", (double)MSHR_used_times/misses*100);
  printf("MAF used times  : %llu\n", MAF_used_times);
  printf("MAF used rate   : %.3f%%\n", (double)MAF_used_times/MSHR_used_times*100);
  printf("Stall times due to MSHR          : %llu\n", stall_MSHR);
  printf("Stall times due to Request Queue : %llu\n", stall_RequestQueue);
  printf("Cache replaced num (not dirty)   : %llu\n", non_dirty_replaced);
  printf("================\n");
  for(int i = 0;i<cache->bank_size;i++){
    printf("Bank %d accesses : %llu\n",i, cache->cache_bank[i].access_num);
    printf("Bank %d misses   : %lld\n",i, cache->cache_bank[i].miss_num);
    printf("Bank %d hits    : %lld\n",i, cache->cache_bank[i].hit_num);
    printf("Bank %d writebacks : %llu\n",i, cache->cache_bank[i].writeback_num);
    printf("Bank %d used rate: %.3f%%\n",i, (double)cache->cache_bank[i].access_num/accesses*100);
    printf("Bank %d hit rate : %.3f%%\n",i, (double)cache->cache_bank[i].hit_num/cache->cache_bank[i].access_num*100);
    printf("Bank %d miss rate: %.3f%%\n",i, (double)cache->cache_bank[i].miss_num/cache->cache_bank[i].access_num*100);
    printf("Bank %d MSHR used times : %llu\n",i, cache->cache_bank[i].MSHR_used_times);
    printf("Bank %d MSHR used rate  : %.3f%%\n",i, (double)cache->cache_bank[i].MSHR_used_times/cache->cache_bank[i].miss_num*100);
    printf("Bank %d MAF used times  : %llu\n",i, cache->cache_bank[i].MAF_used_times);
    printf("Bank %d MAF used rate   : %.3f%%\n",i, (double)cache->cache_bank[i].MAF_used_times/cache->cache_bank[i].MSHR_used_times*100);
    printf("Bank %d stall times due to MSHR : %llu\n",i, cache->cache_bank[i].stall_MSHR_num);
    printf("================\n");
  }
}
