#include "IPC.h"
#include "cachebank.h"

cache_t* cache;         // Data structure for the cache
int cache_block_size;         // Block size
int cache_num_sets;           // Number of sets
int cache_ways;               // cache_ways
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
Queue** trafficTableQueue;
bool noxim_finish = false;  // Indicate if NOXIM input is finish
bool cache_finish = false;  // Indicate if cache finish all request
bool trafficTable_finish = true; // Indicate if traffic table finish all request
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
//! Traffic Queue number
int traffic_queue_num = 0;
//! Input count
int input_count = 0;

void init_global_parameter();
void trafficTableStatus();
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
    executeRemainTraffic();
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
    if(bank_num >= 4){
        traffic_queue_num = 4;
        mutex = (pthread_mutex_t*)malloc(4*sizeof(pthread_mutex_t));
        trafficTableQueue = (Queue**)malloc(4*sizeof(Queue*));
        for(int i=0;i<4;i++){
            pthread_mutex_init(&mutex[i], NULL);
            trafficTableQueue[i] = createQueue(DEFAULT_QUEUE_CAPACITY);
        }
    }
    else{
        for(int i=3;i>=bank_num;i--)
            noc_finish[i] = true;
        mutex = (pthread_mutex_t*)malloc(bank_num*sizeof(pthread_mutex_t));
        traffic_queue_num = bank_num;
        trafficTableQueue = (Queue**)malloc(bank_num*sizeof(Queue*));
        for(int i=0;i<bank_num;i++){
            pthread_mutex_init(&mutex[i], NULL);
            trafficTableQueue[i] = createQueue(DEFAULT_QUEUE_CAPACITY);
        }
    }
    cache_init(bank_num, block_size, cache_size, ways);
    sendBackPacket_init();
    pthread_mutex_init(&mutex_sendBack, NULL);
    pthread_t t1, t2, t3, t4, t5, t6, t7, t8, t9;
    int id[4] = {4,9,14,19};
    pthread_create(&t1, NULL, checkNoC, (void *)id);  // create reader thread
    pthread_create(&t2, NULL, checkNoC, (void *)(id+1));  // create reader thread
    pthread_create(&t3, NULL, checkNoC, (void *)(id+2));  // create reader thread
    pthread_create(&t4, NULL, checkNoC, (void *)(id+3));  // create reader thread
    pthread_create(&t5, NULL, sentBackToNoC, (void *)id);  // create writer thread
    pthread_create(&t6, NULL, sentBackToNoC, (void *)(id+1));  // create writer thread
    pthread_create(&t7, NULL, sentBackToNoC, (void *)(id+2));  // create writer thread
    pthread_create(&t8, NULL, sentBackToNoC, (void *)(id+3));  // create writer thread
    // pthread_create(&t9, NULL, runCache_noxim, NULL);  // create main thread
    // pthread_join(t9, NULL);  // wait for main thread to finish
    runCache_noxim();
    executeRemainTraffic();
    while(!noc_finish[0] || !noc_finish[1] || !noc_finish[2] || !noc_finish[3]){
        for(int i =0;i<traffic_queue_num;i++){
            if(!noc_finish[i]){
                if(isEmpty(trafficTableQueue[i])){
                    noc_finish[i] = true;
                    printf("There has no traffic to be sent back by Cache NIC %d.\n", i);
                }
                else
                    printf("Traffic Table %d still got %d request to send back.\n", i, trafficTableQueue[i]->size);
            }
        }
        cycles++;
        printf("Cycle: %lld\n", cycles);
        printf("--------\n");
        // getchar();
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
        printf("\n<<READER: %d>>\n", *cache_nic_id/5);
        // printf("SHM_NAME_NOC = %s\n", shm_name);
        // Reading from shared memory object
        printf("(R%d) Wait for NoC to set valid signal.\n", *cache_nic_id/5);
        while(CHECKVALID(ptr) != 1) {
            if(noxim_finish){
                printf("Noxim input finished!\n");
                pthread_exit(NULL);
            }
            setIPC_Ready(ptr);
            usleep(10);
        }
        printf("(R%d) Request received!\n", *cache_nic_id/5);
        pthread_mutex_lock(&mutex[*cache_nic_id/5]);
        ready = CHECKREADY(ptr);
        valid = CHECKVALID(ptr);
        resetIPC_Ready(ptr);
        src_id = GETSRC_ID(ptr);
        dst_id = GETDST_ID(ptr);
        packet_id = GETPACKET_ID(ptr);
        packet.src_id = src_id;
        packet.dst_id = dst_id;
        packet.packet_id = packet_id;
        // printf("(%dR) src_id = %d\n", *cache_nic_id/5, src_id);
        // printf("(%dR) dst_id = %d\n", *cache_nic_id/5, dst_id);
        // printf("(%dR) packet_id = %u\n", *cache_nic_id/5, packet_id);
        for(int i = 0; i < 2; i++) {
            req = GETREQ(ptr, i);
            packet.req[i] = req;
            printf("(%dR) req[%d] = 0x%x\n", *cache_nic_id/5, i, req);
        }
        for(int i = 0; i < 8; i++) {
            data = GETDATA(ptr, i);
            packet.data[i] = data;
            // printf("(%dR) data[%d] = 0x%x\n", *cache_nic_id/5, i, data);
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
        printf("Current access number = %llu\n", accesses);
        printf("<<Reader %d Transaction completed!>>\n", *cache_nic_id/5);
        transPacketToTraffic(packet, *cache_nic_id/5);
        while(CHECKACK(ptr)!=0){
        }
        input_count++;
        if(input_count == INPUT_NUM){
            noxim_finish = true;
            printf("Noxim input finished!\n");
        }
        pthread_mutex_unlock(&mutex[*cache_nic_id/5]);
    }
}

void* sentBackToNoC(void* arg){
    int* cache_nic_id = (int*)arg;
    int id = (*cache_nic_id)/5;
    while(!noc_finish[id]){
        int front = trafficTableQueue[id]->front;
        // if(trafficTableQueue[id]->trafficTable[front].finished)
        //     printf("Traffic table %d front dst id = %d finished.\n", id, trafficTableQueue[id]->trafficTable[front].dst_id);
        if(trafficTableQueue[id]->trafficTable[front].dst_id == (*cache_nic_id) && trafficTableQueue[id]->trafficTable[front].finished){
            // TODO: The traffic id should be verified if req is in same packet, since Cache Simulator may run in out of order.
            uint32_t data_test;
            pthread_mutex_lock(&mutex[id]);
            traffic_t traffic = dequeue(trafficTableQueue[id]);
            printf("(Remove) size of trafficTable %d = %d\n", id, trafficTableQueue[id]->size);
            printf("front = %d, rear = %d\n", trafficTableQueue[id]->front, trafficTableQueue[id]->rear);
            // printf("Remove traffic packet id %d from traffic table %d.\n", traffic.packet_id, id);
            pthread_mutex_unlock(&mutex[id]);
            int validIndex = -1;
            pthread_mutex_lock(&mutex_sendBack);
            for(int i=0;i<SEND_BACK_PACKET_SIZE;i++){
                if(validIndex == -1 && sendBackPacket[i].packet_id == -1){
                    //* Setting valid index in case that there is no existed packet in sendBackPacket
                    validIndex = i;
                }
                if(traffic.packet_id == sendBackPacket[i].packet_id){
                    //* The packet is already existed in sendBackPacket
                    validIndex = i;
                    break;
                }
            }
            // printf("The index is %d\n", validIndex);
            // printf("Data is 0x%08x\n", traffic.data);
            // printf("Is tail? %s\n", traffic.tail ? "true" : "false");
            // printf("------------\n");
            if(!traffic.tail){
                if(traffic.req_size == 0){
                    sendBackPacket[validIndex].req[0] = traffic.addr >> 32;
                    sendBackPacket[validIndex].req[1] = traffic.addr;
                    sendBackPacket[validIndex].src_id = traffic.src_id;
                    sendBackPacket[validIndex].dst_id = traffic.dst_id;
                    sendBackPacket[validIndex].packet_id = traffic.packet_id;
                    sendBackPacket[validIndex].read = traffic.req_type;
                }
                sendBackPacket[validIndex].data[traffic.req_size] = traffic.data;
                sendBackPacket[validIndex].request_size = sendBackPacket[validIndex].request_size + 1;
                pthread_mutex_unlock(&mutex_sendBack);
            }
            else{
                sendBackPacket[validIndex].data[traffic.req_size] = traffic.data;
                sendBackPacket[validIndex].request_size = sendBackPacket[validIndex].request_size + 1;
                packet_data packet = sendBackPacket[validIndex];
                resetSendBackPacket(&sendBackPacket[validIndex]);
                pthread_mutex_unlock(&mutex_sendBack);
                printf("\n<<Writer: %d>>\n", id);
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
                // printf("NIC %d Wait for NoC to set ready signal.\n", id);
                while(CHECKREADY(ptr) != 1){
                }
                // printf("NoC is ready to read.\n");
                //* Setting the valid bit and size
                // set src_id
                setIPC_Data(ptr, packet.dst_id, 0, 0);
                printf("(%dW) src_id = %d\n", id, packet.dst_id);
                // set dst_id
                setIPC_Data(ptr, packet.src_id, 1, 0);
                printf("(%dW) dst_id = %d\n", id, packet.src_id);
                // set packet_id
                setIPC_Data(ptr, packet.packet_id, 2, 0);
                printf("(%dW) packet_id = %u\n", id, packet.packet_id);
                // set request addr
                for(int i=0;i<(REQ_PACKET_SIZE/32);i++){
                    setIPC_Data(ptr, packet.req[i], 3, i);
                    printf("(%dW) req[%d] = 0x%08x\n", id, i, packet.req[i]);
                }
                // set request data
                for(int i=0;i<(DATA_PACKET_SIZE/32);i++){
                    setIPC_Data(ptr, packet.data[i], 5, i);
                }
                // set request size (the read data size)
                setIPC_Data(ptr, packet.request_size, 14, 0);
                // set read
                setIPC_Data(ptr, packet.read, 13, 0);
                setIPC_Valid(ptr);
                //! Check if there is any request to be sent back when Cache Simulator finishs all its work
                //! Otherwise, set finish signal for this Cache NIC IPC channel
                // printf("Wait for NoC to set ack signal.\n");
                // printf("================\n");
                // for(int i =0;i<16;i++){
                //     data_test = GETTEST(ptr, i);
                //     printf("(%d) data_test[%02d] = 0x%08x\n", id, i, data_test);
                // }
                // getchar();
                while(CHECKACK(ptr) != 1){
                }
                resetIPC_Valid(ptr);
                resetIPC_Ack(ptr);
                // printf("NoC ack signal is sent back.\n");
                // for(int i =0;i<4;i++)
                //     printf("Traffic Table %d still got %d request to send back.\n", i, trafficTableQueue[i]->size);
                printf("<<Writer %d Transaction completed!>>\n", id);
            }
            if(cache_finish){
                // printf("Traffic Table is empty? %d\n", isEmpty(trafficTableQueue[id]));
                if(isEmpty(trafficTableQueue[id])){
                    noc_finish[id] = true;
                    printf("There has no traffic to be sent back by Cache NIC %d.\n", id);
                }
            }
        }
    }
}

void runCache_noxim(){
    while(!noxim_finish || !trafficTable_finish){
        int stall_counter = 0;
        for(int i =0;i<traffic_queue_num;i++){
            if(isEmpty(trafficTableQueue[i])){
                stall_counter++;
            }
        }
        if(stall_counter != traffic_queue_num){
            cycles++;
            // printf("\n=========\n");
            // printf("Cycle: %lld\n", cycles);
            int input_status;
            // * Load input to request queue
            for(int i =0;i<traffic_queue_num;i++){
                pthread_mutex_lock(&mutex[i]);
                // pthread_mutex_lock(&mutex[0]);
                // pthread_mutex_lock(&mutex[1]);
                // pthread_mutex_lock(&mutex[2]);
                // pthread_mutex_lock(&mutex[3]);
                input_status = checkTrafficTable(trafficTableQueue[i], i);
                // pthread_mutex_unlock(&mutex[0]);
                // pthread_mutex_unlock(&mutex[1]);
                // pthread_mutex_unlock(&mutex[2]);
                // pthread_mutex_unlock(&mutex[3]);
                pthread_mutex_unlock(&mutex[i]);
                if(input_status == 0){
                    printf("Traffic Table %d is empty.\n", i);
                }
                else if(input_status == 2){
                    stall_RequestQueue++;
                    printf("Request Queue of Traffic Table %d is full.\n", i);
                    break;
                }
                else if(input_status == 3){
                    printf("No request of Traffic Table %d can be processed now.\n", i);
                    break;
                }
                else{
                    // printf("No traffic from Traffic Table %d can be processed now.\n", i);
                    // printf("Traffic Table %d size = %d\n", i, trafficTableQueue[i]->size);
                    // printf("Traffic Table %d front = %d\n", i, trafficTableQueue[i]->front);
                    // printf("Traffic finished? %d\n", trafficTableQueue[i]->trafficTable[trafficTableQueue[i]->front].finished);
                    // printf("\n");
                    // trafficTableStatus();
                    printf("Request of Traffic Table %d is sent\n", i);
                }
                // printf("---------\n");
            }
            // * Check the MSHR & MAF status for each bank
            for(int j=0;j<cache->bank_size;j++){
                // ! Only need to check MSHR status when MSHR enabled
                if(cache->cache_bank[j].mshr_queue->enable_mshr){
                    // printf("Checking MSHR %d status...\n", j);
                    // Update the status of MSHR queue of each bank
                    mshr_queue_check_isssue(cache->cache_bank[j].mshr_queue);
                    mshr_queue_counter_add(cache->cache_bank[j].mshr_queue);
                    mshr_queue_check_data_returned(cache->cache_bank[j].mshr_queue);
                    // Check if data in each MSHR entries is returned
                    printf("Status of Bank %d:\n", j);
                    printf("Bank %d is stalled? %s\n", j, cache->cache_bank[j].stall ? "T" : "F");
                    printf("Stall type: %d\n", cache->cache_bank[j].stall_type);
                    for(int i=0; i<cache->cache_bank[j].mshr_queue->entries;i++){
                        // printf("MSHR entry %d: Valid(%s), Issued(%s), Returned(%s)\n", i, cache->cache_bank[j].mshr_queue->mshr[i].valid ? "T" : "F", cache->cache_bank[j].mshr_queue->mshr[i].issued ? "T" : "F", cache->cache_bank[j].mshr_queue->mshr[i].data_returned ? "T" : "F");
                        if(cache->cache_bank[j].mshr_queue->mshr[i].data_returned && cache->cache_bank[j].mshr_queue->mshr[i].valid){
                            printf("Bank %d MSHR Queue %d addr %016llx returned. (Tag is %x)\n", j, i, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, cache->cache_bank[j].mshr_queue->mshr[i].tag);
                            // ! Check if the return addr is satlling the cache bank
                            if(cache->cache_bank[j].stall){
                                if(cache->cache_bank[j].stall_type){
                                    // pthread_mutex_lock(&mutex[0]);
                                    // pthread_mutex_lock(&mutex[1]);
                                    // pthread_mutex_lock(&mutex[2]);
                                    // pthread_mutex_lock(&mutex[3]);
                                    printf("Bank %d is stalled by tag %d\n", j, cache->cache_bank[j].stall_tag);
                                    for(int i =0;i<cache->cache_bank[j].mshr_queue->entries;i++){
                                        printf("MSHR %d tag = %d\n", i, cache->cache_bank[j].mshr_queue->mshr[i].tag);
                                        printf("MSHR %d valid = %s\n", i, cache->cache_bank[j].mshr_queue->mshr[i].valid ? "T" : "F");
                                        printf("MSHR %d issued = %s\n", i, cache->cache_bank[j].mshr_queue->mshr[i].issued ? "T" : "F");
                                        printf("MSHR %d data_returned = %s\n", i, cache->cache_bank[j].mshr_queue->mshr[i].data_returned ? "T" : "F");
                                        printf("----------------\n");
                                    }
                                    // getchar();
                                    // pthread_mutex_unlock(&mutex[0]);
                                    // pthread_mutex_unlock(&mutex[1]);
                                    // pthread_mutex_unlock(&mutex[2]);
                                    // pthread_mutex_unlock(&mutex[3]);
                                    if(cache->cache_bank[j].stall_tag == cache->cache_bank[j].mshr_queue->mshr[i].tag){
                                        cache->cache_bank[j].stall = false;
                                        cache->cache_bank[j].stall_type = 0;
                                        cache->cache_bank[j].stall_addr = 0;
                                        cache->cache_bank[j].stall_tag = 0;
                                        cache->cache_bank[j].stall_traffic = NULL;
                                    }
                                }
                                else{
                                    printf("Bank %d is stalled by any addr.\n", j);
                                    cache->cache_bank[j].stall = false;
                                }
                            }
                            // printf("Request addr %llx returned from MSHR queue %d\n", cache->cache_bank[j].mshr_queue->mshr[i].block_addr, i);
                            // Load returned data to cache set
                            cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].block_addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, addr_mode, cache->cache_bank[j].mshr_queue->mshr[i].maf[0].req_number_on_trace);
                        }
                    }
                    printf("--------\n");
                }
                // * When request queue is not empty, send request to cache bank
                if(!req_queue_empty(cache->cache_bank[j].request_queue)){
                    // printf("Sending request to bank %d\n", j);
                    // printf("Remaining request in bank %d = %d\n", j, cache->cache_bank[j].request_queue->req_num);
                    req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
                }
                else{
                    // printf("Request queue of bank %d is empty.\n", j);
                }
                // printf("--------\n");
                // * Deal with unresolved MAF
            }
        }  
        trafficTable_finish = true;
        for(int i =0;i<traffic_queue_num;i++){
            if(trafficTableQueue[i]->unsent_req != 0){
                trafficTable_finish = false;
                break;
            }
        }
        if(noxim_finish && trafficTable_finish){
            pthread_mutex_lock(&mutex[0]);
            pthread_mutex_lock(&mutex[1]);
            pthread_mutex_lock(&mutex[2]);
            pthread_mutex_lock(&mutex[3]);
            printf("\n-------------------------\n");
            printf("Traffic Table is empty.\n");
            printf("Cycle: %lld\n", cycles);
            printf("\n-------------------------\n");
            getchar();
            pthread_mutex_unlock(&mutex[0]);
            pthread_mutex_unlock(&mutex[1]);
            pthread_mutex_unlock(&mutex[2]);
            pthread_mutex_unlock(&mutex[3]);
        }
    }
}

void executeRemainTraffic(){
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
    printf("\n---------------------\n");
    printf("Cache Simulator finished.\n");
    printf("Total cycles: %lld\n", cycles);
    printf("\n---------------------\n");
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

void transPacketToTraffic(packet_data packet, int id){
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
        enqueue(trafficTableQueue[id], traffic);
        // printf("(Add) size of trafficTable %d = %d\n", id, trafficTableQueue[id]->size);
        // printf("src_id = %d\n", traffic.src_id);
        // printf("dst_id = %d\n", traffic.dst_id);
        // printf("packet_id = %u\n", traffic.packet_id);
        // printf("Addr = %016llx\n", traffic.addr);
        // // printf("req_size = %d\n", req_size);
        // printf("unsent_req = %d\n", trafficTableQueue[id]->unsent_req);
        // printf("front = %d, rear = %d\n", trafficTableQueue[id]->front, trafficTableQueue[id]->rear);
        if(traffic.req_size >= 8){
            pthread_mutex_lock(&mutex[0]);
            pthread_mutex_lock(&mutex[1]);
            pthread_mutex_lock(&mutex[2]);
            pthread_mutex_lock(&mutex[3]);
            printf("Error! Traffic req size too big!\n");
            printf("Traffic req size = %d\n", traffic.req_size);
            getchar();
            pthread_mutex_unlock(&mutex[0]);
            pthread_mutex_unlock(&mutex[1]);
            pthread_mutex_unlock(&mutex[2]);
            pthread_mutex_unlock(&mutex[3]);
        }
        // printf("--------\n");
    }
    printf("unsent_req = %d\n", trafficTableQueue[id]->unsent_req);
    printf("front = %d, rear = %d\n", trafficTableQueue[id]->front, trafficTableQueue[id]->rear);
    printf("Transfer packet to traffic table %d completed!\n", id);
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

void resetSendBackPacket(packet_data* packet){
    packet->src_id = 0;
    packet->dst_id = 0;
    packet->packet_id = -1;
    for(int i = 0;i<8;i++){
        if(i<2)
            packet->req[i] = 0;
        packet->data[i] = 0;
    }
    packet->read = 0;
    packet->request_size = 0;
}

void sendBackPacket_init(){
    sendBackPacket = (packet_data*)malloc(SEND_BACK_PACKET_SIZE*sizeof(packet_data));
    for(int i=0;i<SEND_BACK_PACKET_SIZE;i++){
        sendBackPacket[i].src_id = 0;
        sendBackPacket[i].dst_id = 0;
        sendBackPacket[i].packet_id = -1;
        for(int i = 0;i<8;i++){
            if(i<2)
                sendBackPacket[i].req[i] = 0;
            sendBackPacket[i].data[i] = 0;
        }
        sendBackPacket[i].read = 0;
        sendBackPacket[i].request_size = 0;
    }
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

void trafficTableStatus(){
    pthread_mutex_lock(&mutex[0]);
    pthread_mutex_lock(&mutex[1]);
    pthread_mutex_lock(&mutex[2]);
    pthread_mutex_lock(&mutex[3]);
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
    pthread_mutex_unlock(&mutex[0]);
    pthread_mutex_unlock(&mutex[1]);
    pthread_mutex_unlock(&mutex[2]);
    pthread_mutex_unlock(&mutex[3]);
}