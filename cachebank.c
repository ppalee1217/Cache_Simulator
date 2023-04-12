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
counter_t miss_cycles = 0;  // Total number of cycles due to misses

cache_t* cache;         // Data structure for the cache
int cache_block_size;         // Block size
int cache_size;         // Cache size
int cache_ways;               // cache_ways
int cache_num_sets;           // Number of sets
int cache_num_offset_bits;    // Number of offset bits
int cache_num_index_bits;     // Number of cache_set_index bits

/**
 * Function to intialize your cache simulator with the given cache parameters. 
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
  printf("cache size = %d\n",cache_size);
  printf("cache set num = %d\n",cache_num_sets);
  for(int i=0;i<_bank_size;i++) {
    cache_bank[i].set_num = cache_num_sets/4;
    cache_bank[i].cache_set = cacheset_init(cache_block_size, cache_size/_bank_size, cache_ways);
    printf("cache bank %d set num = %d\n",i,cache_num_sets/4);
    printf("cache bank %d cache size = %d\n",i,cache_size/_bank_size);
    cache_bank[i].mshr_queue = init_mshr_queue(i ,MSHR_ENTRY, MAF_ENTRY);
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
 * @return 0 when error or EOF and 1 otherwise. 
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
        index_mask = (index_mask << 1) + 1;
      }
      cache_set_index = ((address >> (cache_num_offset_bits+2)) & index_mask);
      printf("cache_set_index = %d\n",cache_set_index);
      // getchar();
      // if(cache_set_index < cache_num_sets/4){
      //   printf("Choose bank 0\n");
      //   cacheset_access(cache->cache_bank[0].cache_set ,cache ,0 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
      // }
      // else if(cache_set_index < cache_num_sets/2){
      //   printf("Choose bank 1\n");
      //   cacheset_access(cache->cache_bank[1].cache_set ,cache ,1 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
      // }
      // else if(cache_set_index < cache_num_sets*3/4){
      //   printf("Choose bank 1\n");
      //   cacheset_access(cache->cache_bank[1].cache_set ,cache ,1 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
      // }
      // else{
      //   printf("Choose bank 3\n");
      //   cacheset_access(cache->cache_bank[3].cache_set ,cache , 3,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
      // }
      switch (cache_set_index & 0xF)
      {
        case 0:
        case 1:
        case 2:
        case 3:
        printf("Choose bank 0\n");
          cacheset_access(cache->cache_bank[0].cache_set ,cache ,0 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
          break;
        case 4:
        case 5:
        case 6:
        case 7:
        printf("Choose bank 1\n");
          cacheset_access(cache->cache_bank[1].cache_set ,cache ,1 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
          break;
        case 8:
        case 9:
        case 10:
        case 11:
        printf("Choose bank 2\n");
          cacheset_access(cache->cache_bank[2].cache_set ,cache ,2 ,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
          break;
        case 12:
        case 13:
        case 14:
        case 15:
        printf("Choose bank 3\n");
          cacheset_access(cache->cache_bank[3].cache_set ,cache , 3,address , t, 0, &hits, &misses, &writebacks, &cycles, &miss_cycles);
          break;
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
    while (next_line(input));
    printf("Instruction input complete. Clearing MSHR Queue...\n");
    // printf("Cycle: %lld\n", cycles);
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
    }
    printf("=====================\n");
    bool MSHR_clear = false;
    bool MSHR_not_clear;
    while(!MSHR_clear){
        MSHR_not_clear = false;
        for(int j = 0;j<cache->bank_size;j++){
          for(int i=0;i<cache->cache_bank[j].mshr_queue->entries;i++){
            if(cache->cache_bank[j].mshr_queue->mshr[i].valid)
              MSHR_not_clear = true;
            // Clear the MSHR
            if(cache->cache_bank[j].mshr_queue->mshr[i].valid && cache->cache_bank[j].mshr_queue->mshr[i].data_returned){
              printf("Clearing instruction of Bank %d MSHR Queue %d\n",j,i);
              cacheset_load_MSHR_data(cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue, i),&writebacks);
            }
          }
        }
        if(MSHR_not_clear){
          MSHR_clear = false;
          cycles++;
          miss_cycles++;
          for(int j = 0;j<cache->bank_size; j++){
            mshr_queue_check_isssue(cache->cache_bank[j].mshr_queue);
            mshr_queue_counter_add(cache->cache_bank[j].mshr_queue);
            mshr_queue_check_data_returned(cache->cache_bank[j].mshr_queue);
          }
          printf("- Cycle: %lld\n", cycles);
        }
        else
          MSHR_clear = true;
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
  printf("Accesses       : %llu\n", accesses);
  printf("Hits           : %llu\n", hits); 
  printf("Misses         : %llu\n", misses); 
  printf("Writebacks     : %llu\n", writebacks); 
  printf("Cycles         : %llu\n", cycles);
  printf("Miss cycles    : %llu\n", miss_cycles);
  printf("Hit rate       : %.3f%%\n", (double)hits/accesses*100);
  printf("Miss rate      : %.3f%%\n", (double)misses/accesses*100);
  printf("Writeback rate : %.3f%%\n", (double)writebacks/accesses*100);
}