#include "cachebank.h"

traffic_t* traffic_init(){
    traffic_t* traffic = (traffic_t*) malloc(sizeof(traffic_t));
    traffic->data = (int*) malloc(MAX_FLIT_WORDSIZE*sizeof(int));
    traffic->addr = 0;
    traffic->req_type = 0;
    traffic->src_id = 0;
    traffic->dst_id = 0;
    traffic->packet_size = 0;
    traffic->sequence_no = -1;
    traffic->packet_id = 0;
    traffic->tensor_id = 0;
    traffic->packet_num = 0;
    traffic->flit_word_num = 0;
    traffic->valid = false;
    traffic->working = false;
    traffic->finished = false;
    traffic->tail = false;
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
    request_queue->req_num++;
    // printf("load request to request queue\n");
    // printf("Request tag = %x\n", tag);
    // getchar();
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
                if(running_mode == 2){
                    cache_bank[choose].stall_traffic->finished = true;
                    // if(cache_bank[choose].stall_traffic->tensor_id==2 || cache_bank[choose].stall_traffic->tensor_id==5 || cache_bank[choose].stall_traffic->tensor_id==8 || cache_bank[choose].stall_traffic->tensor_id==11)
                    //     printf("(1)tensor id %d is finished\n", cache_bank[choose].stall_traffic->tensor_id);
                    // printf("(1)Traffic packet id %d finished\n", cache_bank[choose].stall_traffic->packet_id);
                    if(cache_bank[choose].stall_traffic->req_type)
                        for(int i =0;i<cache_bank[choose].stall_traffic->flit_word_num;i++)
                            cache_bank[choose].stall_traffic->data[i] = READ_DATA;
                }
                req_queue_forward(request_queue);
            }
        }
        return;
    }
    else{
        // printf("Cache bank %d is not stalling\n", choose);
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
            // printf("(Bank%d) Send request to cache set\n",choose);
            int maf_result = cacheset_access(cache_bank[choose].cache_set, cache, choose, request_queue->request_addr[0], request_queue->request_type[0], 0, &hits, &misses, &writebacks, addr_mode, request_queue->req_number_on_trace[0], request_queue->traffic[0]);
            // printf("Cacheset back!\n");
            // printf("(Bank%d) MAF result = %d\n",choose, maf_result);
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
                // printf("Request is already in MSHR queue, but MAF is full\n");
                // printf("Stall request addr = %016llx\n", cache_bank[choose].stall_addr);
                // printf("Stall request tag = %x\n", cache_bank[choose].stall_tag);
                // printf("Stall request type = %d\n", cache_bank[choose].stall_type);
                // printf("Stall request traffic = %d\n", cache_bank[choose].stall_traffic->packet_id);
                return;
            }
            else{
                // * MSHR queue is full, wait for any data to return, and then wait for the MAF queue to be cleared
                cache_bank[choose].stall = true;
                cache_bank[choose].stall_type = 0;
                cache_bank[choose].stall_traffic = request_queue->traffic[0];
                // * For non-MSHR design
                cache_bank[choose].stall_addr = request_queue->request_addr[0];
                cache_bank[choose].stall_tag = request_queue->tag[0];
                cache_bank[choose].inst_type = request_queue->request_type[0];
                if(cache_bank[choose].mshr_queue->enable_mshr){
                    stall_MSHR++;
                    cache_bank[choose].stall_MSHR_num++;
                }
                // printf("MSHR queue is full\n");
                // printf("Stall request type = %d\n", cache_bank[choose].stall_type);
                // printf("Stall request addr = %016llx\n", cache_bank[choose].stall_addr);
                // printf("Stall request tag = %x\n", cache_bank[choose].stall_tag);
                // printf("Stall request traffic = %d\n", cache_bank[choose].stall_traffic->packet_id);
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
 * Read the global traffic table and check if there is any request to be issued to the cache.
 * 
 * @return 0 when traffic table is empty or no request can be processed, 1 when a request is processed, 2 when request queue is full.
 */
int checkTrafficTable(Queue* trafficTable, int nic_id){
    if(isEmpty(trafficTable)){
        return 0;
    }
    else if(trafficTable->unsent_req == 0){
        return 3;
    }
    else{
        int index = -1;
        int tmp_rear;
        (trafficTable->rear < trafficTable->front) ? (tmp_rear = trafficTable->rear + trafficTable->capacity) : (tmp_rear = trafficTable->rear);
        for(int i=trafficTable->front;i<=tmp_rear;i++){
            i%=trafficTable->capacity;
            if(trafficTable->trafficTable[i].valid && !trafficTable->trafficTable[i].working){
                index = i;
                break;
            }
        }
        if(index == -1){
            for(int i=trafficTable->front;i<=trafficTable->rear;i++){
                printf("Traffic %d: valid = %d, working = %d, finished = %d, tail = %d\n", i, trafficTable->trafficTable[i].valid, trafficTable->trafficTable[i].working, trafficTable->trafficTable[i].finished, trafficTable->trafficTable[i].tail);
            }
            printf("Error! There is no unsent request to be handled for queue %d.\n", nic_id);
            exit(EXIT_FAILURE);
        }
        traffic_t* traffic = &(trafficTable->trafficTable[index]);
        if(traffic->valid && !traffic->working){
            // printf("Traffic Table %d load traffic into request queue\n", nic_id);
            unsigned int index_mask = 0;
            unsigned int cache_set_index = 0;
            unsigned int tag = 0;
            for(int i=0;i<cache_num_index_bits;i++){
                cache_set_index = (cache_set_index << 1) + 1;
                index_mask = (index_mask << 1) + 1;
            }
            cache_set_index = ((traffic->addr >> (cache_num_offset_bits+2)) & index_mask);
            tag = (traffic->addr >> (cache_num_offset_bits+2+cache_num_index_bits));
            // printf("Cache set index = %x\n", cache_set_index);
            // printf("Cache num offset bits = %d\n", cache_num_offset_bits);
            // printf("Cache num index bits = %d\n", cache_num_index_bits);
            // printf("Traffic addr = %016llx\n", traffic->addr);
            // printf("Traffic tag = %x\n", tag);
            // getchar();
            if(addr_mode == 0){
                int choose_bank = cache_set_index % bank_num;
                // printf("Choose bank = %d\n", choose_bank);
                if(req_queue_full(cache->cache_bank[choose_bank].request_queue)){
                    return 2;
                }
                traffic->working = true;
                req_number_on_trace++;
                cache->cache_bank[choose_bank].access_num++;
                load_request(cache->cache_bank[choose_bank].request_queue, traffic->addr, tag, traffic->req_type, traffic);
                trafficTable->unsent_req = trafficTable->unsent_req - 1;
            }
            else if(addr_mode == 1){
                unsigned int address_partition = ((unsigned int)-1) / bank_num;
                int choose_bank = (traffic->addr / address_partition);
                // printf("Choose bank = %d\n", choose_bank);
                if(req_queue_full(cache->cache_bank[choose_bank].request_queue)){
                    return 2;
                }
                traffic->working = true;
                req_number_on_trace++;
                cache->cache_bank[choose_bank].access_num++;
                load_request(cache->cache_bank[choose_bank].request_queue, traffic->addr, tag, traffic->req_type, traffic);
                trafficTable->unsent_req = trafficTable->unsent_req - 1;
            }
            else if(addr_mode == 2){
                // TODO: add new address mode
            }
            accesses++;
        }
    }
    return 1;
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
    unsigned int* data = (unsigned int*) malloc(8*sizeof(unsigned int));
    unsigned int index_mask = 0;
    unsigned int cache_set_index = 0;
    unsigned int tag = 0;
    instr = 0;
    fscanf(trace, "%d %llx %x %x %x %x %x %x %x %x\n", &t, &address, &data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6], &data[7]);
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