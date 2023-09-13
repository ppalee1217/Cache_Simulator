#include "utils.h"

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

void* new2d(int h, int w, int size)
{
    int i;
    void **p;

    p = (void**)new char[h*sizeof(void*) + h*w*size];
    for(i = 0; i < h; i++)
        p[i] = ((char *)(p + h)) + i*w*size;

    return p;
}

char* concatenateStrings(const char* str1, const char* str2) {
    // Calculate the lengths of the input strings
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);

    // Allocate memory for the concatenated string (+1 for null terminator)
    char* concatenated = (char*)malloc((len1 + len2 + 1) * sizeof(char));

    // Check if memory allocation was successful
    if (concatenated == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    // Copy the contents of str1 into the concatenated string
    strcpy(concatenated, str1);

    // Append the contents of str2 to the concatenated string
    strcat(concatenated, str2);

    return concatenated;
}

/**
 * Function to open the trace file
 * You do not need to update this function. 
 */
FILE *open_trace(const char *filename) {
    return fopen(filename, "r");
}

void init_global_parameter(){
    addr_mode = ADDRESSING_MODE;
    miss_cycle = MISS_CYCLE;
    req_queue_size = REQ_QUEUE_SIZE;
    MSHR_size = MSHR_ENTRY;
    MAF_size = MAF_ENTRY;
    bank_num = BANK_SIZE;
    block_size = BLOCK_SIZE;
    cache_size = CACHE_SIZE;
    ways = WAYS;
}

/**
 * Function to printf cache statistics.
 */
void cache_print_stats() {
    printf("\n\n=============== Cache Simulator Result ===============\n\n");
    printf("------------ Configuration ------------\n");
    printf("Address mode    : %d\n", addr_mode);
    printf("Cache Size      : %d KB\n", cache_size/1024);
    printf("Block Size      : %d B\n", block_size);
    printf("Miss Penalty    : %d cycles\n", miss_cycle);
    printf("Request Queue   : %d\n", req_queue_size);
    if(MSHR_size > 0){
        printf("MSHR Entry      : %d\n", MSHR_size);
        printf("MAF Entry       : %d\n", MAF_size);
    }
    else{
        printf("MSHR Entry      : Disable\n");
        printf("MAF Entry       : Disable\n");
    }
    printf("Bank Number     : %d\n", bank_num);
    printf("%d-Way Set Associative\n\n", ways);
    printf("------------- Statistics -------------\n");
    printf("Cycles          : %llu\n", cycles);
    printf("Accesses        : %llu\n", accesses);
    printf("Hits            : %llu\n", hits); 
    printf("Misses          : %llu\n", misses); 
    printf("Writebacks      : %llu\n", writebacks); 
    printf("Hit rate        : %.3f%%\n", (double)hits/accesses*100);
    printf("Miss rate       : %.3f%%\n", (double)misses/accesses*100);
    //  printf("Writeback rate  : %.3f%%\n", (double)writebacks/accesses*100);
    printf("MSHR used times : %llu\n", MSHR_used_times);
    printf("MSHR used rate  : %.3f%%\n", (double)MSHR_used_times/accesses*100);
    printf("MAF used times  : %llu\n", MAF_used_times);
    printf("MAF used rate   : %.3f%%\n", (double)MAF_used_times/accesses*100);
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
        printf("Bank %d MSHR used rate  : %.3f%%\n",i, (double)cache->cache_bank[i].MSHR_used_times/cache->cache_bank[i].access_num*100);
        printf("Bank %d MAF used times  : %llu\n",i, cache->cache_bank[i].MAF_used_times);
        printf("Bank %d MAF used rate   : %.3f%%\n",i, (double)cache->cache_bank[i].MAF_used_times/cache->cache_bank[i].access_num*100);
        printf("Bank %d stall times due to MSHR : %llu\n",i, cache->cache_bank[i].stall_MSHR_num);
        printf("================\n");
    }
}

void trafficTableStatus(){
    for(int i=0;i<traffic_queue_num;i++){
        printf("Traffic Table %d status:\n", i);
        printf("Traffic Table %d front: %d\n", i, trafficTableQueue[i]->front);
        printf("Traffic Table %d rear: %d\n", i, trafficTableQueue[i]->rear);
        printf("Traffic Table %d size: %d\n", i, trafficTableQueue[i]->size);
        printf("Traffic Table %d capacity: %d\n", i, trafficTableQueue[i]->capacity);
        printf("Traffic src id of front: %x\n", trafficTableQueue[i]->trafficTable[trafficTableQueue[i]->front].src_id);
        printf("--------------------\n");
    }
    getchar();
}