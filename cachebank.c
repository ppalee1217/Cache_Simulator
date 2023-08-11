#include "cachebank.h"

traffic_t* traffic_init(){
    traffic_t* traffic = (traffic_t*) malloc(sizeof(traffic_t));
    traffic->addr = 0;
    traffic->data = 0;
    traffic->req_type = 0;
    traffic->req_size = 0;
    traffic->src_id = 0;
    traffic->dst_id = 0;
    traffic->packet_id = 0;
    traffic->valid = false;
    traffic->working = false;
    traffic->finished = false;
    traffic->noxim_finish = false;
    return traffic;
}

request_queue_t* req_queue_init(int _queue_size){
  request_queue_t* request_queue = (request_queue_t*) malloc(sizeof(request_queue_t));
  request_queue->request_addr = (addr_t*) malloc(_queue_size*sizeof(addr_t));
  request_queue->tag = (unsigned int*) malloc(_queue_size*sizeof(unsigned int));
  request_queue->request_type = (int*) malloc(_queue_size*sizeof(int));
  request_queue->req_number_on_trace = (int*) malloc(_queue_size*sizeof(int));
  request_queue->queue_size = _queue_size;
  request_queue->traffic = (traffic_t**) malloc(_queue_size*sizeof(traffic_t*));
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
  for(int i=0;i<_bank_size;i++) {
    cache_bank[i].set_num = cache_num_sets/4;
    cache_bank[i].cache_set = cacheset_init(cache_block_size, cache_size/_bank_size, cache_ways);
    cache_bank[i].mshr_queue = init_mshr_queue(i ,MSHR_size, MAF_size);
    cache_bank[i].request_queue = req_queue_init(req_queue_size);
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

void load_request(request_queue_t* request_queue, addr_t addr, unsigned int tag, int type, traffic_t* traffic){
    request_queue->request_addr[request_queue->req_num] = addr;
    request_queue->tag[request_queue->req_num] = tag;
    request_queue->request_type[request_queue->req_num] = type;
    request_queue->req_number_on_trace[request_queue->req_num] = req_number_on_trace;
    request_queue->traffic[request_queue->req_num] = traffic;
    // printf("Traffic is loaded to request queue at %d\n", request_queue->req_num);
    // printf("Test1:\n");
    // printf("  src: %d\n", traffic->src_id);
    // printf("  dst: %d\n", traffic->dst_id);
    // printf("  packet id: %d\n", traffic->packet_id);
    // printf("  reqs: %llx\n", traffic->addr);
    // printf("  req type: %d\n", traffic->req_type);
    // printf("  req size: %d\n", traffic->req_size);
    // printf("  valid: %d\n", traffic->valid);
    // printf("  working: %d\n", traffic->working);
    // printf("  finished: %d\n", traffic->finished);
    // printf("======================================\n");
    // printf("  src: %d\n", request_queue->traffic[request_queue->req_num]->src_id);
    // printf("  dst: %d\n", request_queue->traffic[request_queue->req_num]->dst_id);
    // printf("  packet id: %d\n", request_queue->traffic[request_queue->req_num]->packet_id);
    // printf("  reqs: %llx\n", request_queue->traffic[request_queue->req_num]->addr);
    // printf("  req type: %d\n", request_queue->traffic[request_queue->req_num]->req_type);
    // printf("  req size: %d\n", request_queue->traffic[request_queue->req_num]->req_size);
    // printf("  valid: %d\n", request_queue->traffic[request_queue->req_num]->valid);
    // printf("  working: %d\n", request_queue->traffic[request_queue->req_num]->working);
    // printf("  finished: %d\n", request_queue->traffic[request_queue->req_num]->finished);
    // request_queue->traffic[request_queue->req_num]->finished = true;
    // printf("\nTest2:\n");
    // printf("Traffic:\n");
    // printf("  src: %d\n", traffic->src_id);
    // printf("  dst: %d\n", traffic->dst_id);
    // printf("  packet id: %d\n", traffic->packet_id);
    // printf("  reqs: %llx\n", traffic->addr);
    // printf("  req type: %d\n", traffic->req_type);
    // printf("  req size: %d\n", traffic->req_size);
    // printf("  valid: %d\n", traffic->valid);
    // printf("  working: %d\n", traffic->working);
    // printf("  finished: %d\n", traffic->finished);
    // printf("======================================\n");
    // printf("  src: %d\n", request_queue->traffic[request_queue->req_num]->src_id);
    // printf("  dst: %d\n", request_queue->traffic[request_queue->req_num]->dst_id);
    // printf("  packet id: %d\n", request_queue->traffic[request_queue->req_num]->packet_id);
    // printf("  reqs: %llx\n", request_queue->traffic[request_queue->req_num]->addr);
    // printf("  req type: %d\n", request_queue->traffic[request_queue->req_num]->req_type);
    // printf("  req size: %d\n", request_queue->traffic[request_queue->req_num]->req_size);
    // printf("  valid: %d\n", request_queue->traffic[request_queue->req_num]->valid);
    // printf("  working: %d\n", request_queue->traffic[request_queue->req_num]->working);
    // printf("  finished: %d\n", request_queue->traffic[request_queue->req_num]->finished);
    // request_queue->traffic[request_queue->req_num]->finished = false;
    request_queue->req_num++;
}

bool req_queue_full(request_queue_t* request_queue){
  if(request_queue->req_num == request_queue->queue_size)
    return true;
  else
    return false;
}

bool req_queue_empty(request_queue_t* request_queue){
  if(request_queue->req_num == 0)
    return true;
  else
    return false;
}

void req_queue_forward(request_queue_t* request_queue){
    for(int i=0;i<request_queue->req_num-1;i++){
        request_queue->request_addr[i] = request_queue->request_addr[i+1];
        request_queue->tag[i] = request_queue->tag[i+1];
        request_queue->request_type[i] = request_queue->request_type[i+1];
        request_queue->req_number_on_trace[i] = request_queue->req_number_on_trace[i+1];
        request_queue->traffic[i] = request_queue->traffic[i+1];
    }
    request_queue->request_addr[request_queue->req_num-1] = 0;
    request_queue->tag[request_queue->req_num-1] = 0;
    request_queue->request_type[request_queue->req_num-1] = 0;
    request_queue->req_number_on_trace[request_queue->req_num-1] = 0;
    request_queue->req_num = request_queue->req_num - 1;
    request_queue->traffic[request_queue->req_num] = NULL;
    // printf("Request queue is forwarded\n");
}

void req_send_to_set(cache_t* cache, request_queue_t* request_queue, cache_bank_t* cache_bank, int choose){
    //* Check if this cache bank is stalling for MSHR Queue
    //* if true, update the MSHR & MAF info
    //* else, send the request to cache set
    if(cache_bank[choose].stall){
        // ! Check stall status of non-MSHR design 
        if(!cache_bank[choose].mshr_queue->enable_mshr){
            cache_bank[choose].stall_counter++;
            // * Miss data returned, load data to cache
            if(cache_bank[choose].stall_counter == miss_cycle){
                // printf("Stall request is back, load data to cache\n");
                cache_bank[choose].stall = false;
                cache_bank[choose].stall_counter = 0;
                cacheset_load_MSHR_data(cache_bank[choose].set_num, choose, cache, cache_bank[choose].cache_set, cache_bank[choose].stall_addr, cache_bank[choose].inst_type, &writebacks, &non_dirty_replaced, addr_mode, 0);
                cache_bank[choose].stall_traffic->finished = true;
                printf("Traffic data %x finished\n", cache_bank[choose].stall_traffic->data);
                req_queue_forward(request_queue);
            }
        }
        return;
    }
    else{
        unsigned int index = 0;
        for(int i=0;i<cache_num_index_bits;i++){
            index = (index << 1) + 1;
        }
        index = ((request_queue->request_addr[0] >> (cache_num_offset_bits+2)) & index);
        // Setting offset
        unsigned int offset = 0;
        for(int i=0;i<cache_num_offset_bits;i++){
            offset = (offset << 1) + 1;
        }
        offset = ((request_queue->request_addr[0] >> 2) & offset);
        // Check if the request queue is empty
        if(!req_queue_empty(request_queue)){
            int maf_result = cacheset_access(cache_bank[choose].cache_set, cache, choose, request_queue->request_addr[0], request_queue->request_type[0], 0, &hits, &misses, &writebacks, addr_mode, request_queue->req_number_on_trace[0], request_queue->traffic[0]);
            // printf("Cacheset back!\n");
            // printf("MAF result = %d\n", maf_result);
            // Check the MSHR Queue
            if(maf_result == 3){
                // * hit
            }
            else if(maf_result == 1){
                // request is successfully logged and MAF is not full
                // => check if the request is issued to DRAM, and log MAF (is done by mshr_queue_get_entry)
                MSHR_used_times++;
                cache_bank[choose].MSHR_used_times++;
            }
            else if(maf_result == 2){
                // request is already logged but MAF is not full
                // => check if the request is issued to DRAM, and log MAF (is done by mshr_queue_get_entry)
                MAF_used_times++;
                cache_bank[choose].MAF_used_times++;
            }
            else if(maf_result == -1){
                // request is already in the queue, but MAF is full
                // => check if the request is issued to DRAM, and wait for this request to be done(stall => while loop)
                // * Wait for data to return, and clear 1 instruction so that this new instruction can be logged.
                // * meantime, MSHR will continue handle the returned data unexecuted instruction
                
                // ! Update request queue stall info
                cache_bank[choose].stall = true;
                cache_bank[choose].stall_type = 1;
                cache_bank[choose].stall_addr = request_queue->request_addr[0];
                cache_bank[choose].stall_tag = request_queue->tag[0];
                cache_bank[choose].stall_traffic = request_queue->traffic[0];
                if(cache_bank[choose].mshr_queue->enable_mshr){
                    stall_MSHR++;
                    cache_bank[choose].stall_MSHR_num++;
                }
                return;
            }
            else{
                // * MSHR queue is full, wait for any data to return, and then wait for the MAF queue to be cleared
                cache_bank[choose].stall = true;
                cache_bank[choose].stall_type = 0;
                // * For non-MSHR design
                cache_bank[choose].stall_addr = request_queue->request_addr[0];
                cache_bank[choose].stall_tag = request_queue->tag[0];
                cache_bank[choose].inst_type = request_queue->request_type[0];
                cache_bank[choose].stall_traffic = request_queue->traffic[0];
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
 * Read the global traffic table and check if there is any request to be issued to the cache.
 * 
 * @return 0 when traffic table is empty or no request can be processed, 1 when a request is processed, 2 when request queue is full.
 */
int checkTrafficTable(){
    if(isEmpty(trafficTableQueue)){
        return 0;
    }
    else{
        traffic_t* traffic = &(trafficTableQueue->trafficTable[trafficTableQueue->front]);
        if(traffic->valid && !traffic->working){
            unsigned int index_mask = 0;
            unsigned int cache_set_index = 0;
            unsigned int tag = 0;
            for(int i=0;i<cache_num_index_bits;i++){
                cache_set_index = (cache_set_index << 1) + 1;
                index_mask = (index_mask << 1) + 1;
            }
            cache_set_index = ((traffic->addr >> (cache_num_offset_bits+2)) & index_mask);
            tag = (traffic->addr >> (cache_num_offset_bits+2+cache_num_index_bits));
            if(addr_mode == 0){
                int choose_bank = cache_set_index % bank_num;
                printf("Choose bank = %d\n", choose_bank);
                if(req_queue_full(cache->cache_bank[choose_bank].request_queue)){
                    return 2;
                }
                traffic->working = true;
                req_number_on_trace++;
                cache->cache_bank[choose_bank].access_num++;
                load_request(cache->cache_bank[choose_bank].request_queue, traffic->addr, 0, traffic->req_type, traffic);
                if(traffic->noxim_finish){
                    accesses++;
                    noxim_finish = true;
                    printf("Noxim finish!\n");
                    return 3;
                }
            }
            else if(addr_mode == 1){
                unsigned int address_partition = ((unsigned int)-1) / bank_num;
                int choose_bank = (traffic->addr / address_partition);
                printf("Choose bank = %d\n", choose_bank);
                if(req_queue_full(cache->cache_bank[choose_bank].request_queue)){
                    return 2;
                }
                traffic->working = true;
                req_number_on_trace++;
                cache->cache_bank[choose_bank].access_num++;
                load_request(cache->cache_bank[choose_bank].request_queue, traffic->addr, 0, traffic->req_type, traffic);
                if(traffic->noxim_finish){
                    accesses++;
                    noxim_finish = true;
                    printf("Noxim finish!\n");
                    return 3;
                }
            }
            else if(addr_mode == 2){

            }
            accesses++;
            return 1;
        }
    }
    return 0;
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
    unsigned int tag = 0;
    fscanf(trace, "%d %llx %llx\n", &t, &address, &instr);
    for(int i=0;i<cache_num_index_bits;i++){
      cache_set_index = (cache_set_index << 1) + 1;
      index_mask = (index_mask << 1) + 1;
    }
    cache_set_index = ((address >> (cache_num_offset_bits+2)) & index_mask);
    tag = (address >> (cache_num_offset_bits+2+cache_num_index_bits));
    if(addr_mode == 0){
      int choose_bank = cache_set_index % bank_num;
      if(req_queue_full(cache->cache_bank[choose_bank].request_queue)){
        fseek(trace, file_pointer_temp, SEEK_SET);
        return 2;
      }
      else{
        req_number_on_trace++;
        cache->cache_bank[choose_bank].access_num++;
        load_request(cache->cache_bank[choose_bank].request_queue, address, tag, t, NULL);
      }
    }
    else if(addr_mode == 1){
      unsigned int address_partition = ((unsigned int)-1) / bank_num;
      int choose_bank = address / address_partition;
      if(req_queue_full(cache->cache_bank[choose_bank].request_queue)){
        fseek(trace, file_pointer_temp, SEEK_SET);
        return 2;
      }
      else{
        req_number_on_trace++;
        cache->cache_bank[choose_bank].access_num++;
        load_request(cache->cache_bank[choose_bank].request_queue, address, tag, t, NULL);
      }
    }
    else if(addr_mode == 2){

    }
    accesses++;
  }
  return 1;
}