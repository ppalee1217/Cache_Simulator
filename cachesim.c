/**
 * @author ECE 3058 TAs
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "cachesim.h"
#include "mshr.h"

// Statistics to keep track.
counter_t accesses = 0;     // Total number of cache accesses
counter_t hits = 0;         // Total number of cache hits
counter_t misses = 0;       // Total number of cache misses
counter_t writebacks = 0;   // Total number of writebacks
counter_t cycles = 0;       // Total number of cycles
counter_t miss_cycles = 0;  // Total number of cycles due to misses

/**
 * Function to perform a very basic log2. It is not a full log function, 
 * but it is all that is needed for this assignment. The <math.h> log
 * function causes issues for some people, so we are providing this. 
 * 
 * @param x is the number you want the log of.
 * @returns Techinically, floor(log_2(x)). But for this lab, x should always be a power of 2.
 */
int simple_log_2(int x) {
    int val = 0;
    while (x > 1) {
        x /= 2;
        val++;
    }
    return val; 
}

cache_set_t* cache;     // Data structure for the cache
int block_size;         // Block size
int cache_size;         // Cache size
int ways;               // Ways
int num_sets;           // Number of sets
int num_offset_bits;    // Number of offset bits
int num_index_bits;     // Number of index bits

/**
 * Function to intialize your cache simulator with the given cache parameters. 
 * Note that we will only input valid parameters and all the inputs will always 
 * be a power of 2.
 * 
 * @param _block_size is the block size in bytes
 * @param _cache_size is the cache size in bytes
 * @param _ways is the associativity
 */
void cachesim_init(int _block_size, int _cache_size, int _ways) {
    // Set cache parameters to global variables
    block_size = _block_size;   // In byte
    cache_size = _cache_size;   // In byte
    ways = _ways;
    // cache size = block size * ways * number of sets
    num_sets = cache_size / (block_size * ways);
    // num_offset_bits: the offset of desired data in cache block(if cache block size > 32)
    // this variable stands for bits number(size) in addr_t
    num_offset_bits = simple_log_2((block_size/4));
    // num_index_bits: the index of desired cache set
    // this variable stands for bits number(size) in addr_t
    num_index_bits = simple_log_2(num_sets);
        // printf("num_sets: %d\n",num_sets);
        // printf("num_offset_bits: %d\n",num_offset_bits);
        // printf("num_index_bits: %d\n",num_index_bits);
        // printf("Offset in bit: %x\n",offset);
        // printf("Index in bit: %x\n",index);
        // printf("[1:0]: Byte offset\n");
        // printf("[%d:2]: Block offset\n",num_offset_bits + 1);
        // printf("[%d:%d]: Cache index\n",num_offset_bits + num_index_bits + 1,num_offset_bits + 2);
        // printf("[64:%d]: Tag\n",num_offset_bits + num_index_bits + 2);
    // Initializing cache
    cache = (cache_set_t*) malloc(num_sets * sizeof(cache_set_t));
    for(int i=0;i<num_sets;i++){
        cache[i].size = ways;
        // Initializing lru_stack
        cache[i].stack = init_lru_stack(ways);
        // Initializing mshr_queue
        cache[i].mshr_queue = init_mshr_queue(MSHR_ENTRY, MAF_ENTRY);
        // Initializing blocks
        cache[i].blocks = (cache_block_t*) malloc(ways * sizeof(cache_block_t));
        for(int j=0;j<ways;j++){
            cache[i].blocks[j].valid = false;
            cache[i].blocks[j].dirty = false;
            cache[i].blocks[j].tag = 0;
        }
        // cache->blocks->data = (int*) malloc((block_size/32) * sizeof(int));
    }
}

/**
 * Function to perform a SINGLE memory access to your cache. In this function, 
 * you will need to update the required statistics (accesses, hits, misses, writebacks)
 * and update your cache data structure with any changes necessary.
 * 
 * @param physical_addr is the address to use for the memory access. 
 * @param access_type is the type of access - 0 (data read), 1 (data write) or 
 *      2 (instruction read). We have provided macros (MEMREAD, MEMWRITE, IFETCH)
 *      to reflect these values in cachesim.h so you can make your code more readable.
 * @param destination is the PE index that requested the access.
 */
void cachesim_access(addr_t physical_addr, int access_type, unsigned int destination) {
    // addr_t is a 64-bit unsigned integer.
    // Encoding:
    // [1:0]                                     : Byte offset
    // [num_offset_bits + 1: 2]                  : Data to fetch in cache block(if cache block size > 32)
    // [num_offset_bits + num_index_bits + 1]    : num_offset_bits + 2]: Cache set index
    // [64: num_offset_bits + num_index_bits + 2]: Tag

    // 1.
    // Read access type:
    // Distinguish between data read, data write, and instruction fetch

    // 2.
    // First, find cache set from [num_offset_bits + num_index_bits + 1: num_offset_bits + 2]
    // if the cache set is n-way, compare n cache blocks' tag with [64: num_offset_bits + num_index_bits + 2]

    // 3.
    // When read/write data, update LRU stack by lru_stack_set_mru
    // we need to compare if the data is already in one of the cache blocks
    // If so, pass the index of the block to lru_stack_set_mru
    // If not, pass the index of not valid block to lru_stack_set_mru
    // If cache set is full, e.g, every block inside it is valid
    // get the LRU block index from lru_stack_get_lru
    // and then replace the LRU block with the new block
    //
    // Note: issue to concern when replacing the LRU block
    // if the LRU block is dirty, we need to write back to memory first before replacing it
    // (1) Read data: need to use block index to choose which block portion to be read
    // (2) Write data: need to update dirty bit
    // (3) Inst. read: (not support if time is short)

    // Issue to handle now: bitwise operation in C to get index & offset in address
    accesses++;
    cycles++;
    printf("- Cycle: %lld\n", cycles);
    unsigned int offset = 0;
    unsigned int index = 0;
    int tag = 0;
    for(int i=0;i<num_offset_bits;i++){
        offset = (offset << 1) + 1;
    }
    for(int i=0;i<num_index_bits;i++){
        index = (index << 1) + 1;
    }
    offset = ((physical_addr >> 2) & offset);
    index = ((physical_addr >> (num_offset_bits+2)) & index);
    tag = (physical_addr >> (num_index_bits + num_offset_bits + 2));
    // printf("address = %llx\n",physical_addr);
    // printf("offset = %x\n",offset);
    // printf("index = %x\n",index);
    // printf("tag = %x\n",tag);
    // printf("=====================\n");
        // for(int i=0;i<ways;i++){
        //     printf("Cache[%d] Block[%d] = %x, valid = %d, Priority = %d\n",index,i,cache[index].blocks[i].tag,cache[index].blocks[i].valid,cache[index].stack->priority[i]);
        // }
    // ! Issued the request in MSHR queue (1 only in 1 cycle)
    // !!!!!!! wrong
    mshr_queue_check_isssue(cache->mshr_queue);
    // ! Add counter of every mshr queue entry that issued
    mshr_queue_counter_add(cache->mshr_queue);
    // ! Check if the requested data is returned
    mshr_queue_check_data_returned(cache->mshr_queue);
    // ! Log the new returned data to cache & clear 1 instruction
    for(int i=0; i<cache->mshr_queue->entries;i++){
        if(cache->mshr_queue[i].mshr->data_returned && cache->mshr_queue[i].mshr->valid){
            printf("Clearing instruction of MSHR Queue %d\n",i);
            cachesim_load_MSHR_data(cache->mshr_queue[i].mshr->block_addr, mshr_queue_clear_inst(cache->mshr_queue,i));
        }
    }
    // Check if new data hit or miss
    bool hit;
    int hit_index = 0;
    for(int i=0;i<ways;i++){
        hit = false;
        if(cache[index].blocks[i].tag == tag && cache[index].blocks[i].valid){
            hit = true;
            hit_index = i;
            printf("Hit! tag: %x is inside cache %d block %d\n",tag,index,i);
            break;
        }
    }
    if(hit){
        // hit
        hits++;
        // Read: consider block index
        
        // Write: need to update dirty bit also
        if(access_type == MEMWRITE)
            cache[index].blocks[hit_index].dirty = true;
        // inst. read:

        // Update LRU
        lru_stack_set_mru(cache[index].stack, hit_index);
    }
    else{
        printf("Miss! tag: %x is not inside cache\n",tag);
        // miss
        misses++;
        int maf_result = mshr_queue_get_entry(cache->mshr_queue, physical_addr, access_type, offset, destination);
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
            miss_cycles++;
            bool data_back = false;
            while(!mshr_queue_check_specific_data_returned(cache->mshr_queue, physical_addr) && !data_back){
                printf("Waiting for specific data to return...\n");
                cycles++;
                miss_cycles++;
                mshr_queue_check_isssue(cache->mshr_queue);
                mshr_queue_counter_add(cache->mshr_queue);
                mshr_queue_check_data_returned(cache->mshr_queue);
                for(int i =0 ;i<cache->mshr_queue->entries;i++){
                    if(cache->mshr_queue[i].mshr->valid && cache->mshr_queue[i].mshr->data_returned){
                        printf("Clearing instruction of MSHR Queue %d\n",i);
                        cachesim_load_MSHR_data(cache->mshr_queue[i].mshr->block_addr, mshr_queue_clear_inst(cache->mshr_queue,i));
                        if(cache->mshr_queue[i].mshr->block_addr == physical_addr)
                            data_back = true;
                    }
                }
            }
            // * log the info of unlogged MAF
            mshr_queue_get_entry(cache->mshr_queue, physical_addr, access_type, offset, destination);
            printf("- Cycle: %lld\n", cycles);
        }
        else{
            //* printf("This request is not in MSHR Queue.\n");
            miss_cycles++;
            // * MSHR queue is full, wait for any data to return, and then wait for the MAF queue to be cleared
            while(mshr_queue_get_entry(cache->mshr_queue, physical_addr, access_type, offset, destination) != 1){
                // printf("Waiting for any data to return & clear...\n");
                cycles++;
                miss_cycles++;
                mshr_queue_check_isssue(cache->mshr_queue);
                mshr_queue_counter_add(cache->mshr_queue);
                mshr_queue_check_data_returned(cache->mshr_queue);
                for(int i =0 ;i<cache->mshr_queue->entries;i++){
                    if(cache->mshr_queue[i].mshr->valid && cache->mshr_queue[i].mshr->data_returned){
                        printf("Clearing instruction of MSHR Queue %d\n",i);
                        cachesim_load_MSHR_data(cache->mshr_queue[i].mshr->block_addr, mshr_queue_clear_inst(cache->mshr_queue,i));
                    }
                }
                printf("- Cycle: %lld\n", cycles);
            }
            // ! log the info of returned data to cache
            // request is not in the queue
        }
        printf("=====================\n");
    }
}

void cachesim_load_MSHR_data(addr_t physical_addr, int access_type){
    printf("-> Addr: %llx Type: %d\n",physical_addr,access_type);
    bool replace_block;
    unsigned int offset = 0;
    unsigned int index = 0;
    int tag = 0;
    for(int i=0;i<num_offset_bits;i++){
        offset = (offset << 1) + 1;
    }
    for(int i=0;i<num_index_bits;i++){
        index = (index << 1) + 1;
    }
    offset = ((physical_addr >> 2) & offset);
    index = ((physical_addr >> (num_offset_bits+2)) & index);
    tag = (physical_addr >> (num_index_bits + num_offset_bits + 2));

    for(int i=0;i<ways;i++){
        replace_block = true;
        if(!cache[index].blocks[i].valid){
            // Cache is not full yet
            replace_block = false;
            // Update cache
            cache[index].blocks[i].valid = true;
            cache[index].blocks[i].tag = tag;
            if(access_type == MEMWRITE)
                cache[index].blocks[i].dirty = true;
            // Update LRU
            lru_stack_set_mru(cache[index].stack, i);
            break;
        }
    }
    if(replace_block){
        // Cache is full
        int lru_index = lru_stack_get_lru(cache[index].stack);
        // Check if replacing block is dirty
            //printf("Full! block %d is replacing\n",lru_index);
        if(cache[index].blocks[lru_index].dirty){
            // ! Not consider writeback cycles yet
            writebacks++;
        }
        // Update cache
        if(access_type == MEMWRITE)
            cache[index].blocks[lru_index].dirty = true;
        else
            cache[index].blocks[lru_index].dirty = false;
        cache[index].blocks[lru_index].tag = tag;
        cache[index].blocks[lru_index].valid = true;
        // Update LRU
        lru_stack_set_mru(cache[index].stack, lru_index);
    }
}
/**
 * Function to free up any dynamically allocated memory.
 */
void cachesim_cleanup() {
    lru_stack_cleanup(cache->stack);
    mshr_queue_cleanup(cache->mshr_queue);
    free(cache->blocks);
    free(cache);
}

/**
 * Function to print cache statistics.
 */
void cachesim_print_stats() {
    printf("With %d ways associativity, %d sets, and %d bytes per block\n", ways, num_sets, block_size);
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
        fscanf(trace, "%d %llx %llx\n", &t, &address, &instr);
        cachesim_access(address, t, 0);
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
                        " <cache size(bytes)> <ways>\n", argv[0]);
        return 1;
    }
    
    input = open_trace(argv[1]);
    cachesim_init(atol(argv[2]), atol(argv[3]), atol(argv[4]));
    while (next_line(input));
    printf("Instruction input complete. Clearing MSHR Queue...\n");
    // printf("Cycle: %lld\n", cycles);
    for(int i = 0;i<cache->mshr_queue->entries;i++){
        if(cache->mshr_queue[i].mshr->valid){
            printf("MSHR Queue %d is not clear\n",i);
        }
        else{
            printf("MSHR Queue %d is clear\n",i);
        }
    }
    printf("=====================\n");
    bool MSHR_clear = false;
    bool MSHR_not_clear;
    while(!MSHR_clear){
        MSHR_not_clear = false;
        for(int i=0;i<cache->mshr_queue->entries;i++){
            if(cache->mshr_queue[i].mshr->valid)
                MSHR_not_clear = true;
            // Clear the MSHR
            if(cache->mshr_queue[i].mshr->valid && cache->mshr_queue[i].mshr->data_returned){
                printf("Clearing instruction of MSHR Queue %d\n",i);
                cachesim_load_MSHR_data(cache->mshr_queue[i].mshr->block_addr, mshr_queue_clear_inst(cache->mshr_queue,i));
            }
        }
        if(MSHR_not_clear){
            MSHR_clear = false;
            cycles++;
            miss_cycles++;
            mshr_queue_check_isssue(cache->mshr_queue);
            mshr_queue_counter_add(cache->mshr_queue);
            mshr_queue_check_data_returned(cache->mshr_queue);
            printf("- Cycle: %lld\n", cycles);
        }
        else
            MSHR_clear = true;
    }
    cachesim_print_stats();
    cachesim_cleanup();
    fclose(input);
    return 0;
}
