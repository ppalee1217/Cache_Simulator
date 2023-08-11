#include "IPC.h"
#include "cachebank.h"

cache_t* cache;         // Data structure for the cache
int cache_block_size;         // Block size
int cache_ways;               // cache_ways
int cache_num_sets;           // Number of sets
int cache_num_offset_bits;    // Number of block bits
int cache_num_index_bits;     // Number of cache_set_index bits
unsigned long long file_pointer_temp; // Temp file pointer for trace file
int req_number_on_trace = 0;
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
bool enable = false;              // Enable debug mode
//! For NOXIM
Queue* trafficTableQueue;
bool noxim_finish = false;  // Indicate if NOXIM input is finish
bool cache_finish = false;  // Indicate if cache finish all request
bool noc_finish[4] = {false, false, false, false}; // Indicate if noc finish all sent back request
//! Global parameters
int addr_mode;
int miss_cycle;
int req_queue_size;
int MSHR_size;
int MAF_size;
int bank_num;
int block_size;
int cache_size;
int ways;
//! Running mode
int running_mode;

void init_global_parameter();

/**
 * Main function. See error message for usage. 
 * 
 * @param argc number of arguments
 * @param argv Argument values
 * @returns 0 on success. 
 */
int main(int argc, char **argv) {
    printf("Welcome to Cache Simulator!\n");
    init_global_parameter();
    noxim_finish = false;
    cache_finish = false;
    printf("1. Trace mode\n");
    printf("2. Noxim mode\n");
    printf("Default mode is trace mode.\n\n");
    printf("Choose running mode: ");
    scanf("%d", &running_mode);
    if(running_mode == 1)
        fedByTrace();
    else if(running_mode == 2)
        fedByNoxim();
    else{
        printf("Invalid input, running in trace mode.\n");
        fedByTrace();
    }
    return 0;
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

void fedByTrace(){
    char trace[50];
    int num_input = 0;
    printf("Running in trace mode.\n");
    printf("Usage:<trace> <block size(bytes)> <cache size(bytes)> <cache_ways>\n");
    num_input = scanf("%s %d %d %d", trace, &block_size, &cache_size, &ways);
    FILE *input;
    if (num_input != 4) {
        fprintf(stderr, "Usage: <trace> <block size(bytes)>"
                        " <cache size(bytes)> <cache_ways>\n");
        return;
    }
    printf("\n");
    input = open_trace(trace);
    cache_init(bank_num, block_size, cache_size, ways);
    int input_status = 1;
    while (input_status != 0){
      cycles++;
      // * Load input to request queue
      for(int i =0;i<cache->bank_size;i++){
        file_pointer_temp = ftell(input);
        input_status = next_line(input);
        if(input_status == 0){
            printf("Instruction input complete.\n");
            break;
        }
        else if(input_status == 2){
            stall_RequestQueue++;
            break;
        }
      }
      // * Check the MSHR & MAF status for each bank
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
              // ! Check if the return addr is satlling the cache bank
              if(cache->cache_bank[j].stall){
                if(cache->cache_bank[j].stall_type){
                  if(cache->cache_bank[j].stall_tag == cache->cache_bank[j].mshr_queue->mshr[i].tag){
                    cache->cache_bank[j].stall = false;
                    cache->cache_bank[j].stall_type = 0;
                    cache->cache_bank[j].stall_addr = 0;
                    cache->cache_bank[j].stall_tag = 0;
                    cache->cache_bank[j].stall_traffic = NULL;
                  }
                }
                else{
                  cache->cache_bank[j].stall = false;
                }
              }
              // Load returned data to cache set
              cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, addr_mode, cache->cache_bank[j].mshr_queue->mshr[i].maf[0].req_number_on_trace);
            }
          }
        }
        // * When request queue is not empty, send request to cache bank
        if(!req_queue_empty(cache->cache_bank[j].request_queue)){
          req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
        }
        // * Deal with unresolved MAF
      }
    }
    executeRemainTraffic(NULL);
    cache_print_stats();
    //* Cleanup
    for(int i=0;i<cache->bank_size;i++){
      cacheset_cleanup(cache->cache_bank[i].cache_set);
      mshr_queue_cleanup(cache->cache_bank[i].mshr_queue);
    }
    free(cache->cache_bank);
    free(cache);
    fclose(input);
    return;
}

void fedByNoxim(){
    //* trafficTableQueue multiple thread protection is not implemented yet
    printf("Running in Noxim mode.\n");
    trafficTableQueue = createQueue(DEFAULT_QUEUE_CAPACITY);
    cache_init(bank_num, block_size, cache_size, ways);
    pthread_t t1, t2, t3, t4, t5, t6, t7, t8, t9;
    int id[4] = {4,9,14,19};
    //! while(!cache_finish || !noc_finish[0] || !noc_finish[1] || !noc_finish[2] || !noc_finish[3]){
    pthread_create(&t1, NULL, checkNoC, (void *)id);  // create reader thread
    // pthread_create(&t2, NULL, checkNoC, (void *)(id+1));  // create reader thread
    // pthread_create(&t3, NULL, checkNoC, (void *)(id+2));  // create reader thread
    // pthread_create(&t4, NULL, checkNoC, (void *)(id+3));  // create reader thread
    pthread_create(&t5, NULL, sentBackToNoC, (void *)id);  // create writer thread
    // pthread_create(&t6, NULL, sentBackToNoC, (void *)(id+1));  // create writer thread
    // pthread_create(&t7, NULL, sentBackToNoC, (void *)(id+2));  // create writer thread
    // pthread_create(&t8, NULL, sentBackToNoC, (void *)(id+3));  // create writer thread
    while(!cache_finish || !noc_finish[0]){
        if(!cache_finish){
            pthread_create(&t9, NULL, runCache_noxim, NULL);  // create main thread
            pthread_join(t9, NULL);  // wait for main thread to finish
        }
    }
    cache_print_stats();
    //* Cleanup
    for(int i=0;i<cache->bank_size;i++){
      cacheset_cleanup(cache->cache_bank[i].cache_set);
      mshr_queue_cleanup(cache->cache_bank[i].mshr_queue);
    }
    free(cache->cache_bank);
    free(cache);
    return;
}

void* checkNoC(void* arg){
    while(!noxim_finish){
        int* cache_nic_id = (int*)arg;
        const char* id_name;
        switch(*cache_nic_id){
            case 4:
                id_name = "4_NOC";
                break;
            case 9:
                id_name = "9_NOC";
                break;
            case 14:
                id_name = "14_NOC";
                break;
            case 19:
                id_name = "19_NOC";
                break;
            default:
                id_name = "4_NOC";
                break;
        }
        const char* shm_name = concatenateStrings(SHM_NAME, id_name);
        // Creating shared memory object
        int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
        // Setting file size
        ftruncate(fd, SHM_SIZE);
        // Mapping shared memory object to process address space
        uint32_t* ptr = (uint32_t*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        memset(ptr, 0, SHM_SIZE);
        uint32_t src_id, dst_id, packet_id, req, data, read, request_size;
        uint32_t data_test;
        uint32_t ready, valid, ack;
        bool finish;
        packet_data packet;
        printf("\n<<READER>>\n");
        // printf("SHM_NAME_NOC = %s\n", shm_name);
        // Reading from shared memory object
        // printf("Wait for NoC to set valid signal.\n");
        while(CHECKVALID(ptr) != 1) {
            if(noxim_finish)
                pthread_exit(NULL);
            setIPC_Ready(ptr);
            usleep(10);
        }
        printf("\n(R) Request received!\n");
        pthread_mutex_lock(&mutex1);
        ready = CHECKREADY(ptr);
        valid = CHECKVALID(ptr);
        resetIPC_Ready(ptr);
        src_id = GETSRC_ID(ptr);
        dst_id = GETDST_ID(ptr);
        packet_id = GETPACKET_ID(ptr);
        packet.src_id = src_id;
        packet.dst_id = dst_id;
        packet.packet_id = packet_id;
        // printf("src_id = %d\n", src_id);
        // printf("dst_id = %d\n", dst_id);
        for(int i = 0; i < 2; i++) {
            req = GETREQ(ptr, i);
            packet.req[i] = req;
            // printf("req[%d] = 0x%x\n", i, req);
        }
        for(int i = 0; i < 8; i++) {
            data = GETDATA(ptr, i);
            packet.data[i] = data;
            // printf("data[%d] = 0x%x\n", i, data);
        }
        read = GETREAD(ptr);
        packet.read = read;
        // printf("read (1 = true): %d\n", read);
        request_size = GETREQUEST_SIZE(ptr);
        packet.request_size = request_size;
        // printf("request_size = %d\n", request_size);
        finish = CHECKFINISH(ptr);
        packet.finish = finish;
        // printf("finish (1 = true): %d\n", finish);
        setIPC_Ack(ptr);
        // data_test = GETTEST(ptr, 15);
        printf("<<Reader Transaction completed!>>\n");
        transPacketToTraffic(packet);
        while(CHECKACK(ptr)!=0){
        }
        pthread_mutex_unlock(&mutex1);
    }
}

void* sentBackToNoC(void* arg){
    int* cache_nic_id = (int*)arg;
    while(!noc_finish[*cache_nic_id/5]){
        uint32_t data_test;
        bool sent = false;
        if(trafficTableQueue->trafficTable[trafficTableQueue->front].src_id == *cache_nic_id && trafficTableQueue->trafficTable[trafficTableQueue->front].finished){
            pthread_mutex_lock(&mutex1);
            traffic_t traffic = dequeue(trafficTableQueue);
            pthread_mutex_unlock(&mutex1);
            if(!traffic.tail){
                if(traffic.req_size == 0){
                    resetsendBackPacket();
                    sendBackPacket.req[0] = traffic.addr >> 32;
                    sendBackPacket.req[1] = traffic.addr;
                }
                sendBackPacket.src_id = traffic.src_id;
                sendBackPacket.dst_id = traffic.dst_id;
                sendBackPacket.packet_id = traffic.packet_id;
                sendBackPacket.data[traffic.req_size] = traffic.data;
                sendBackPacket.read = traffic.req_type;
                sendBackPacket.request_size = sendBackPacket.request_size + 1;
            }
            else{
                sendBackPacket.data[traffic.req_size] = traffic.data;
                sendBackPacket.request_size = sendBackPacket.request_size + 1;
                printf("\n<<Writer>>\n");
                const char* id_name;
                switch(*cache_nic_id){
                    case 4:
                        id_name = "4_CACHE";
                        break;
                    case 9:
                        id_name = "9_CACHE";
                        break;
                    case 14:
                        id_name = "14_CACHE";
                        break;
                    case 19:
                        id_name = "19_CACHE";
                        break;
                    default:
                        id_name = "4_CACHE";
                        break;
                }
                const char* shm_name = concatenateStrings(SHM_NAME, id_name);
                // Creating shared memory object
                int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
                // Setting file size
                ftruncate(fd, SHM_SIZE);
                // Mapping shared memory object to process address space
                uint32_t* ptr = (uint32_t*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                memset(ptr, 0, SHM_SIZE);
                uint32_t ready, valid, ack;
                // printf("SHM_NAME_CACHE = %s\n", shm_name);
                // printf("Wait for NoC to set ready signal.\n");
                while(CHECKREADY(ptr) != 1){
                    // if(cache_finish){
                    //     if(isEmpty(trafficTableQueue)){
                    //         noc_finish[*cache_nic_id/5] = true;
                    //         printf("There is no traffic to be sent back by Cache NIC %d.\n", *cache_nic_id/5);
                    //         pthread_exit(NULL);
                    //     }
                    // }
                }
                // printf("NoC is ready to read.\n");
                //* Setting the valid bit and size
                // set src_id
                setIPC_Data(ptr, sendBackPacket.dst_id, 0, 0);
                // set dst_id
                setIPC_Data(ptr, sendBackPacket.src_id, 1, 0);
                // set packet_id
                setIPC_Data(ptr, sendBackPacket.packet_id, 2, 0);
                // set request addr
                for(int i=0;i<(REQ_PACKET_SIZE/32);i++){
                    setIPC_Data(ptr, sendBackPacket.req[i], 3, i);
                }
                // set request data
                for(int i=0;i<(DATA_PACKET_SIZE/32);i++){
                    setIPC_Data(ptr, sendBackPacket.data[i], 5, i);
                }
                // set request size (the read data size)
                setIPC_Data(ptr, sendBackPacket.request_size, 14, 0);
                // set read
                setIPC_Data(ptr, sendBackPacket.read, 13, 0);
                setIPC_Valid(ptr);
                printf("Remove traffic data %x from traffic queue.\n", traffic.data);
                //! Check if there is any request to be sent back when Cache Simulator finishs all its work
                //! Otherwise, set finish signal for this Cache NIC IPC channel
                if(cache_finish){
                    // if(isEmpty(trafficTableQueue)){
                    //     noc_finish[*cache_nic_id/5] = true;
                    //     printf("There is no traffic to be sent back by Cache NIC %d.\n", *cache_nic_id/5);
                    // }
                }
                // printf("Wait for NoC to set ack signal.\n");
                while(CHECKACK(ptr) != 1){
                }
                resetIPC_Valid(ptr);
                resetIPC_Ack(ptr);
                // printf("NoC ack signal is sent back.\n");
                printf("<<Writer Transaction completed!>>\n");
            }
            if(cache_finish){
                // printf("Traffic Table is empty? %d\n", isEmpty(trafficTableQueue));
                if(isEmpty(trafficTableQueue)){
                    noc_finish[*cache_nic_id/5] = true;
                    printf("There is no traffic to be sent back by Cache NIC %d.\n", *cache_nic_id/5);
                }
            }
        }
    }
}

void* runCache_noxim(void* arg){
    while(!noxim_finish){
        if(!(isEmpty(trafficTableQueue))){
            cycles++;
            int input_status;
            // * Load input to request queue
            for(int i =0;i<cache->bank_size;i++){
                pthread_mutex_lock(&mutex1);
                input_status = checkTrafficTable();
                pthread_mutex_unlock(&mutex1);
                if(input_status == 0){
                    // printf("Traffic Table is empty or no traffic can be processed now.\n");
                }
                else if(input_status == 2){
                    stall_RequestQueue++;
                    break;
                }
                else if(input_status == 3){
                    // This request is the last request from Noxim
                    printf("The input from NOXIM is finished.\n");
                    break;
                }
            }
            // * Check the MSHR & MAF status for each bank
            for(int j=0;j<cache->bank_size;j++){
                // ! Only need to check MSHR status when MSHR enabled
                if(cache->cache_bank[j].mshr_queue->enable_mshr){
                    // printf("Checking MSHR status...\n");
                    // Update the status of MSHR queue of each bank
                    mshr_queue_check_isssue(cache->cache_bank[j].mshr_queue);
                    mshr_queue_counter_add(cache->cache_bank[j].mshr_queue);
                    mshr_queue_check_data_returned(cache->cache_bank[j].mshr_queue);
                    // Check if data in each MSHR entries is returned
                    for(int i=0; i<cache->cache_bank[j].mshr_queue->entries;i++){
                        if(cache->cache_bank[j].mshr_queue->mshr[i].data_returned && cache->cache_bank[j].mshr_queue->mshr[i].valid){
                            // ! Check if the return addr is satlling the cache bank
                            if(cache->cache_bank[j].stall){
                                if(cache->cache_bank[j].stall_type){
                                if(cache->cache_bank[j].stall_tag == cache->cache_bank[j].mshr_queue->mshr[i].tag){
                                    cache->cache_bank[j].stall = false;
                                    cache->cache_bank[j].stall_type = 0;
                                    cache->cache_bank[j].stall_addr = 0;
                                    cache->cache_bank[j].stall_tag = 0;
                                    cache->cache_bank[j].stall_traffic = NULL;
                                }
                                }
                                else{
                                cache->cache_bank[j].stall = false;
                                }
                            }
                            // printf("Request addr %llx returned from MSHR queue %d\n", cache->cache_bank[j].mshr_queue->mshr[i].block_addr, i);
                            printf("cycles = %lld\n", cycles);
                            // Load returned data to cache set
                            cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, addr_mode, cache->cache_bank[j].mshr_queue->mshr[i].maf[0].req_number_on_trace);
                        }
                    }
                }
                // * When request queue is not empty, send request to cache bank
                if(!req_queue_empty(cache->cache_bank[j].request_queue))
                    req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
                // * Deal with unresolved MAF
            }
        }  
    }
    executeRemainTraffic(NULL);
}

void* executeRemainTraffic(void* arg){
    if(running_mode == 2){
        printf("\n<<executeRemainTraffic>>\n");
        printf("trafficTableQueue size: %ld\n", trafficTableQueue->size);
    }
    // ! Execution not done yet => request queue need to be cleared
    printf("Clearing Request Queue & MSHR Queue...\n");
    printf("Cycle: %lld\n", cycles);
    printf("=====================\n");
    //* Print the state of queue in each bank
    for(int j = 0;j<cache->bank_size;j++){
        printf("-------------\n");
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
    printf("---------------------\n");
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
              // ! Check if the tag of return addr is satlling the cache bank
              if(cache->cache_bank[j].stall){
                if(cache->cache_bank[j].stall_type){
                  if(cache->cache_bank[j].stall_tag == cache->cache_bank[j].mshr_queue->mshr[i].tag){
                    cache->cache_bank[j].stall = false;
                    cache->cache_bank[j].stall_type = 0;
                    cache->cache_bank[j].stall_addr = 0;
                    cache->cache_bank[j].stall_tag = 0;
                    cache->cache_bank[j].stall_traffic = NULL;
                  }
                }
                else{
                  cache->cache_bank[j].stall = false;
                }
              }
              //* Handle the data returned MSHR queue inst.
              cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, addr_mode, cache->cache_bank[j].mshr_queue->mshr[i].maf[0].req_number_on_trace);
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
      printf("---------------------\n");
    }
    cache_finish = true;
    printf("Cache Simulator finished.\n");
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

void transPacketToTraffic(packet_data packet){
    bool loop = true;
    int counter = 0;
    int req_size = packet.request_size;
    addr_t req_addr = packet.req[0];
    req_addr <<= 32;
    req_addr |= packet.req[1];
    while(loop){
        traffic_t traffic;
        traffic.addr = req_addr;
        // printf("req_addr = %llx\n", req_addr);
        traffic.data = packet.data[counter];
        // printf("packet.data[%d] = %x\n", counter, traffic.data);
        traffic.req_type = packet.read;
        traffic.req_size = (packet.request_size - req_size);    // req_size became the index number of the request
        traffic.src_id = packet.src_id;
        traffic.dst_id = packet.dst_id;
        traffic.packet_id = packet.packet_id;
        traffic.valid = true;
        traffic.working = false;
        traffic.finished = false;
        traffic.noxim_finish = false;
        traffic.tail = false;
        //* 
        req_size --;
        req_addr += 4;
        counter++;
        if(req_size <= 0){
            loop = false;
            traffic.tail = true;
            traffic.noxim_finish = packet.finish;
        }
        enqueue(trafficTableQueue, traffic);
    }
}

void setIPC_Data(uint32_t *ptr, uint32_t data, int const_pos, int varied_pos){
    *(ptr + const_pos + varied_pos) = data;
    return;
}

void setIPC_Valid(uint32_t *ptr){
    *(ptr + 15) = (*(ptr + 15) | (0b1 << 30));
    return;
}

void resetIPC_Valid(uint32_t *ptr){
    *(ptr + 15) = (*(ptr + 15) & ~(0b1 << 30));
    return;
}

void setIPC_Ready(uint32_t *ptr){
    *(ptr + 15) = (*(ptr + 15) | (0b1 << 31));
    return;
}

void resetIPC_Ready(uint32_t *ptr){
    *(ptr + 15) = (*(ptr + 15) & ~(0b1 << 31));
    return;
}

void setIPC_Ack(uint32_t *ptr){
    *(ptr + 15) = (*(ptr + 15) | (0b1 << 29));
    return;
}

void resetIPC_Ack(uint32_t *ptr){
    *(ptr + 15) = (*(ptr + 15) & ~(0b1 << 29));
    return;
}

void resetsendBackPacket(){
    sendBackPacket.src_id = 0;
    sendBackPacket.dst_id = 0;
    sendBackPacket.packet_id = 0;
    for(int i = 0;i<8;i++){
        if(i<2)
            sendBackPacket.req[i] = 0;
        sendBackPacket.data[i] = 0;
    }
    sendBackPacket.read = 0;
    sendBackPacket.request_size = 0;
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
    printf("MSHR Entry      : %d\n", MSHR_size);
    printf("MAF Entry       : %d\n", MAF_size);
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