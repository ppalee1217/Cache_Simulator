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
counter_t bank0_num = 0;    // Total number of bank0 accesses
counter_t bank1_num = 0;    // Total number of bank1 accesses
counter_t bank2_num = 0;    // Total number of bank2 accesses
counter_t bank3_num = 0;    // Total number of bank3 accesses 

cache_t* cache;         // Data structure for the cache
int cache_block_size;         // Block size
int cache_size;         // Cache size
int cache_ways;               // cache_ways
int cache_num_sets;           // Number of sets
int cache_num_offset_bits;    // Number of block bits
int cache_num_index_bits;     // Number of cache_set_index bits
int direct_map_index_bits;    // Number of direct_map_index bits

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
  direct_map_index_bits = simple_log_2(cache_num_sets*cache_ways);
}

void load_request(request_queue_t* request_queue, addr_t addr, int type){
  request_queue->request_addr[request_queue->req_num] = addr;
  request_queue->request_type[request_queue->req_num] = type;
  request_queue->req_num++;
}

bool req_queue_full(request_queue_t* request_queue){
  if(request_queue->req_num == request_queue->queue_size-1){
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
  for(int i=0;i<request_queue->req_num-1;i++){
    request_queue->request_addr[i] = request_queue->request_addr[i+1];
    request_queue->request_type[i] = request_queue->request_type[i+1];
  }
  request_queue->req_num = request_queue->req_num - 1;
}

void req_send_to_set(request_queue_t* request_queue, cache_bank_t* cache_bank, int choose){
  // Check if this cache bank is stalling for MSHR Queue
  // if true, update the MSHR & MAF info
  // else, send the request to cache set
  if(cache_bank[choose].stall){
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
      int maf_result = cacheset_access(cache_bank->cache_set, cache, choose, request_queue->request_addr[0], request_queue->request_type[0], 0, &hits, &misses, &writebacks);
      // Check the MSHR Queue
      if(maf_result == 1){
        //* printf("This request is not in MSHR Queue, and is logged now\n");
        // request is successfully logged and MAF is not full
        // => check if the request is issued to DRAM, and log MAF (is done by mshr_queue_get_entry)
        // * log instruction to MAF if the data is returned or not
      }
      else if(maf_result == 2){
        //* printf("This request is in MSHR Queue, and MAF is not full yet.\n");
        // request is successfully logged and MAF is not full
        // => check if the request is issued to DRAM, and log MAF (is done by mshr_queue_get_entry)
        // * log instruction to MAF if the data is returned or not
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
      }
      else{
        //* printf("This request is not in MSHR Queue.\n");
        // * MSHR queue is full, wait for any data to return, and then wait for the MAF queue to be cleared
        cache_bank[choose].stall = true;
        cache_bank[choose].stall_type = 0;    
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
    unsigned int real_index_mask = 0;
    unsigned int cache_set_index = 0;
    fscanf(trace, "%d %llx %llx\n", &t, &address, &instr);
    for(int i=0;i<direct_map_index_bits;i++){
      if(i<cache_num_index_bits)
        real_index_mask = (real_index_mask << 1) + 1;
      index_mask = (index_mask << 1) + 1;
    }
    cache_set_index = ((address >> (cache_num_offset_bits+2)) & index_mask);
    printf("Cache_set_index = %d\n",cache_set_index);
    printf("direct map index bit length = %d\n",direct_map_index_bits);
    printf("Actucaly index = %lld\n",((address >> (cache_num_offset_bits+2)) & real_index_mask));
    printf("way associative index bit length = %d\n",cache_num_index_bits);
    // printf("Addr = %p\n",address);
    if(ADDRESSING_MODE == 0){
      switch (cache_set_index & 0x3){
        case 0:
          printf("Choose bank 0\n");
          getchar();
          if(req_queue_full(cache->cache_bank[0].request_queue)){
          // printf("request queue 0 is full\n");
          // printf("trace before fseek: %ld\n",ftell(trace));
            fseek(trace, ftell(trace)-14, SEEK_SET);
          // printf("trace after fseek: %ld\n",ftell(trace));
            return 2;
          }
          else{
            bank0_num++;
            // cacheset_access(cache->cache_bank[0].cache_set ,cache ,0 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
            load_request(cache->cache_bank[0].request_queue, address, t);
          // printf("request is load to queue 0\n");
          }
          break;
        case 1:
          printf("Choose bank 1\n");
          getchar();
          if(req_queue_full(cache->cache_bank[1].request_queue)){
          // printf("request queue 1 is full\n");
          // printf("trace before fseek: %ld\n",ftell(trace));
            fseek(trace, ftell(trace)-14, SEEK_SET);
          // printf("trace after fseek: %ld\n",ftell(trace));
            if(ftell(trace)==4970){
              getchar();
            }
            return 2;
          }
          else{
            bank1_num++;
            // cacheset_access(cache->cache_bank[1].cache_set ,cache ,1 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
            load_request(cache->cache_bank[1].request_queue, address, t);
          // printf("request is load to queue 1\n");
          }
          break;
        case 2:
          printf("Choose bank 2\n");
          getchar();
          if(req_queue_full(cache->cache_bank[2].request_queue)){
          // printf("request queue 2 is full\n");
          // printf("trace before fseek: %ld\n",ftell(trace));
            fseek(trace, ftell(trace)-14, SEEK_SET);
          // printf("trace after fseek: %ld\n",ftell(trace));
            return 2;
          }
          else{
            bank2_num++;
            // cacheset_access(cache->cache_bank[2].cache_set ,cache ,2 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
            load_request(cache->cache_bank[2].request_queue, address, t);
          // printf("request is load to queue 2\n");
          }
          break;
        case 3:
          printf("Choose bank 3\n");
          getchar();
          if(req_queue_full(cache->cache_bank[3].request_queue)){
          // printf("request queue 3 is full\n");
          // printf("trace before fseek: %ld\n",ftell(trace));
            fseek(trace, ftell(trace)-14, SEEK_SET);
          // printf("trace after fseek: %ld\n",ftell(trace));
            return 2;
          }
          else{
            bank3_num++;
            // cacheset_access(cache->cache_bank[3].cache_set ,cache ,3 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
            load_request(cache->cache_bank[3].request_queue, address, t);
          // printf("request is load to queue 3\n");
          }
          break;
      }
    }
    else if(ADDRESSING_MODE == 1){
      if(cache_set_index < (cache_num_sets*cache_ways)/4){
      // printf("Choose bank 0\n");
        if(req_queue_full(cache->cache_bank[0].request_queue)){
        // printf("request queue 0 is full\n");
        // printf("trace before fseek: %ld\n",ftell(trace));
          fseek(trace, ftell(trace)-14, SEEK_SET);
        // printf("trace after fseek: %ld\n",ftell(trace));
          return 2;
        }
        else{
          bank0_num++;
          // cacheset_access(cache->cache_bank[0].cache_set ,cache ,0 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
          load_request(cache->cache_bank[0].request_queue, address, t);
        // printf("request is load to queue 0\n");
        }
      }
      else if(cache_set_index < (cache_num_sets*cache_ways)/2){
      // printf("Choose bank 1\n");
        if(req_queue_full(cache->cache_bank[1].request_queue)){
        // printf("request queue 1 is full\n");
        // printf("trace before fseek: %ld\n",ftell(trace));
          fseek(trace, ftell(trace)-14, SEEK_SET);
        // printf("trace after fseek: %ld\n",ftell(trace));
          return 2;
        }
        else{
          bank1_num++;
          // cacheset_access(cache->cache_bank[1].cache_set ,cache ,1 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
          load_request(cache->cache_bank[1].request_queue, address, t);
        // printf("request is load to queue 1\n");
        }
      }
      else if(cache_set_index < (cache_num_sets*cache_ways)*3/4){
      // printf("Choose bank 1\n");
        if(req_queue_full(cache->cache_bank[2].request_queue)){
        // printf("request queue 2 is full\n");
        // printf("trace before fseek: %ld\n",ftell(trace));
          fseek(trace, ftell(trace)-14, SEEK_SET);
        // printf("trace after fseek: %ld\n",ftell(trace));
          return 2;
        }
        else{
          bank2_num++;
          // cacheset_access(cache->cache_bank[2].cache_set ,cache ,2 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
          load_request(cache->cache_bank[2].request_queue, address, t);
        // printf("request is load to queue 2\n");
        }
      }
      else{
      // printf("Choose bank 3\n");
        if(req_queue_full(cache->cache_bank[3].request_queue)){
        // printf("request queue 3 is full\n");
        // printf("trace before fseek: %ld\n",ftell(trace));
          fseek(trace, ftell(trace)-14, SEEK_SET);
        // printf("trace after fseek: %ld\n",ftell(trace));
          return 2;
        }
        else{
          bank3_num++;
          // cacheset_access(cache->cache_bank[3].cache_set ,cache ,3 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
          load_request(cache->cache_bank[3].request_queue, address, t);
        // printf("request is load to queue 3\n");
        }
      }
    }
    else if(ADDRESSING_MODE == 2){

    }
    accesses++;
  // printf("--------------------\n");
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
      // printf("- Cycles: %lld\n",cycles);
      // printf("--------------------\n");
      // * Load input to request queue
      for(int i =0;i<cache->bank_size;i++){
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
      // * Load each request in queue to Cache bank
      for(int j=0;j<cache->bank_size;j++){
        // Update the status of MSQH queue of each bank
        mshr_queue_check_isssue(cache->cache_bank[j].mshr_queue);
        mshr_queue_counter_add(cache->cache_bank[j].mshr_queue);
        mshr_queue_check_data_returned(cache->cache_bank[j].mshr_queue);
        // Check if data in each MSHR entries is returned
        for(int i=0; i<cache->cache_bank[j].mshr_queue->entries;i++){
          if(cache->cache_bank[j].mshr_queue->mshr[i].data_returned && cache->cache_bank[j].mshr_queue->mshr[i].valid){
          // printf("Clearing instruction of Bank %d MSHR Queue %d\n",j,i);
            // ! Check if return addr is satlling the cache bank
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
            cacheset_load_MSHR_data(cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks);
          }
        }
        if(!req_queue_empty(cache->cache_bank[j].request_queue)){
          // printf("Send request to bank %d (current req num = %d)\n",j,cache->cache_bank[j].request_queue->req_num);
          req_send_to_set(cache->cache_bank[j].request_queue, cache->cache_bank, j);
        }
        else{
          // printf("Bank %d request queue is empty\n",j);
        }
        // printf("--------------------\n");
      }
    // printf("=====================\n");
      // getchar();
    }
    // ! not done yet => request queue need to be cleared
  // printf("Instruction input complete. Clearing Request Queue & MSHR Queue...\n");
    // printf("Cycle: %lld\n", cycles);
    for(int j = 0;j<cache->bank_size;j++){
    // printf("Bank %d:\n",j);
      for(int i = 0;i<cache->cache_bank[j].mshr_queue->entries;i++){
          if(cache->cache_bank[j].mshr_queue->mshr[i].valid){
          // printf("Bank %d MSHR Queue %d is not clear\n",j,i);
          }
          else{
          // printf("Bank %d MSHR Queue %d is clear\n",j,i);
          }
      }
      if(cache->cache_bank[j].request_queue->req_num != 0){
      // printf("Bank %d Request Queue is not clear\n",j);
      }
      else{
      // printf("Bank %d Request Queue is clear\n",j);
      }
    }
  // printf("=====================\n");
    bool MSHR_clear = false;
    bool MSHR_not_clear;
    while(!MSHR_clear){
      MSHR_not_clear = false;
      //* Check if MSHR is clear
      for(int j = 0;j<cache->bank_size;j++){
        for(int i=0;i<cache->cache_bank[j].mshr_queue->entries;i++){
          if(cache->cache_bank[j].mshr_queue->mshr[i].valid)
            MSHR_not_clear = true;
          // Clear the MSHR
          if(cache->cache_bank[j].mshr_queue->mshr[i].valid && cache->cache_bank[j].mshr_queue->mshr[i].data_returned){
          // printf("Clearing instruction of Bank %d MSHR Queue %d\n",j,i);
            // ! Check if return addr is satlling the cache bank
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
            cacheset_load_MSHR_data(cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks);
          }
        }
        // If request queue is not empty, send request to cache bank
        if(!req_queue_empty(cache->cache_bank[j].request_queue))
          req_send_to_set(cache->cache_bank[j].request_queue, cache->cache_bank, j);
      }
      //* If MSHR is not clear, add cycle and check if data returned
      if(MSHR_not_clear){
        cycles++;
      // printf("- Cycles: %lld\n",cycles);
        MSHR_clear = false;
        for(int j = 0;j<cache->bank_size; j++){
          mshr_queue_check_isssue(cache->cache_bank[j].mshr_queue);
          mshr_queue_counter_add(cache->cache_bank[j].mshr_queue);
          mshr_queue_check_data_returned(cache->cache_bank[j].mshr_queue);
        }
        // printf("- Cycle: %lld\n", cycles);
      }
      else
        MSHR_clear = true;
    // printf("=====================\n");
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
 * Function to print cache statistics.
 */
void cache_print_stats() {
  printf("With %d cache_ways associativity, %d sets, and %d bytes per cache set\n", cache_ways, cache_num_sets, cache_block_size);
  printf("Accesses        : %llu\n", accesses);
  printf("Hits            : %llu\n", hits); 
  printf("Misses          : %llu\n", misses); 
  printf("Writebacks      : %llu\n", writebacks); 
  printf("Cycles          : %llu\n", cycles);
  printf("Hit rate        : %.3f%%\n", (double)hits/accesses*100);
  printf("Miss rate       : %.3f%%\n", (double)misses/accesses*100);
  printf("Writeback rate  : %.3f%%\n", (double)writebacks/accesses*100);
  printf("Bank 0 used rate: %.3f%%\n", (double)bank0_num/accesses*100);
  printf("Bank 1 used rate: %.3f%%\n", (double)bank1_num/accesses*100);
  printf("Bank 2 used rate: %.3f%%\n", (double)bank2_num/accesses*100);
  printf("Bank 3 used rate: %.3f%%\n", (double)bank3_num/accesses*100);
}