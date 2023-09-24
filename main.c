#include "cachebank.h"
#include "IPC.h"
#include "globalParameter.h"
#include "datastruct.h"
#include "config.h"

cache_t* cache;                       // Data structure for the cache
int cache_block_size;                 // Block size
int cache_num_sets;                   // Number of sets
int cache_ways;                       // cache_ways
int cache_num_offset_bits;            // Number of block bits
int cache_num_index_bits;             // Number of cache_set_index bits
unsigned long long file_pointer_temp; // Temp file pointer for trace file
int req_number_on_trace = 0;

//* Statistics to keep track.
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

//* For Noxim
Queue** trafficTableQueue;
bool noxim_finish = false;  // Indicate if NOXIM input is finish
bool cache_finish = false;  // Indicate if cache finish all request
bool trafficTable_finish = true; // Indicate if traffic table finish all request
bool noc_finish[NIC_NUM]; // Indicate if noc finish all sent back request

//* Global parameters
int addr_mode;
int miss_cycle;
int req_queue_size;
int MSHR_size;
int MAF_size;
int bank_num;
int block_size;
int cache_size;
int ways;

//* Running mode
int running_mode;
//* Traffic Queue number
int traffic_queue_num;
//* Dependency Table
int Finish_tensorID[FIN_TENSOR_NUM] = {29, 32, 35, 38};
tensorDependcy** tensorDependencyTable;
int sendback_count[2] = {0, 0};

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
              cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, addr_mode, cache->cache_bank[j].mshr_queue->mshr[i].maf[0].req_number_on_trace);
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
    traffic_queue_num = NIC_NUM;
    mutex = (pthread_mutex_t*)malloc(NIC_NUM*sizeof(pthread_mutex_t));
    trafficTableQueue = (Queue**)malloc(NIC_NUM*sizeof(Queue*));
    for(int i=0;i<NIC_NUM;i++){
        noc_finish[i] = false;
        pthread_mutex_init(&mutex[i], NULL);
        trafficTableQueue[i] = createQueue(DEFAULT_QUEUE_CAPACITY);
    }
    tensorDependencyTable = NEW2D(FIN_TENSOR_NUM, NIC_NUM, tensorDependcy);
    for(int i=0;i<FIN_TENSOR_NUM;i++){
        for(int j=0;j<NIC_NUM;j++){
            tensorDependencyTable[i][j].tensor_id = Finish_tensorID[i];
            tensorDependencyTable[i][j].packet_count = -1;
            tensorDependencyTable[i][j].return_flag = false;
        }
    }
    cache_init(bank_num, block_size, cache_size, ways);
    sendBackPacket_init();
    pthread_mutex_init(&mutex_sendBack, NULL);
    pthread_t* t = (pthread_t*)malloc((NIC_NUM*2)*sizeof(pthread_t));
    int* id = (int*) malloc(NIC_NUM*sizeof(int));
    for(int i=0;i<NIC_NUM;i++){
        id[i] = i;
        pthread_create(&t[i], NULL, checkNoC, (void *)(id+i));         // create reader thread
        pthread_create(&t[4+i], NULL, sentBackToNoC, (void *)(id+i));  // create writer thread
    }
    runCache_noxim();
    executeRemainTraffic();
    bool while_loop_flag = true;
    while(while_loop_flag){
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
        int noc_finish_counter = 0;
        for(int i=0;i<NIC_NUM;i++)
            (noc_finish[i]) ? noc_finish_counter++ : noc_finish_counter;
        if(noc_finish_counter == NIC_NUM)
            while_loop_flag = false;
    }
    cache_print_stats();
    // Cleanup
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
        int id = (*cache_nic_id);
        const char* id_name;
        switch(id){
            case 0:
                id_name = "0_NOC";
                break;
            case 1:
                id_name = "1_NOC";
                break;
            case 2:
                id_name = "2_NOC";
                break;
            case 3:
                id_name = "3_NOC";
                break;
            default:
                id_name = "0_NOC";
                break;
        }
        const char* shm_name = concatenateStrings(SHM_NAME, id_name);
        //* Creating shared memory object
        int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
        //* Setting file size
        ftruncate(fd, SHM_SIZE);
        //* Mapping shared memory object to process address space
        uint32_t* ptr = (uint32_t*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        memset(ptr, 0, SHM_SIZE);
        uint32_t src_id, dst_id, packet_id, tensor_id, packet_num, packet_size;
        bool ready, valid, ack;
        //* Initialize packet
        Packet packet;
        //* Reading from shared memory object
        while(CHECKVALID(ptr) != 1) {
            if(noxim_finish){
                pthread_exit(NULL);
            }
            setIPC_Ready(ptr);
            usleep(10);
        }
        pthread_mutex_lock(&mutex[id]);
        ready = CHECKREADY(ptr);
        valid = CHECKVALID(ptr);
        resetIPC_Ready(ptr);
        src_id = GETSRC_ID(ptr);
        dst_id = GETDST_ID(ptr);
        packet_id = GETPACKET_ID(ptr);
        packet_num = GETPACKET_NUM(ptr);
        tensor_id = GETTENSOR_ID(ptr);
        packet_size = GETPACKET_SIZE(ptr);
        packet.src_id = src_id;
        packet.dst_id = dst_id;
        packet.packet_num = packet_num;
        packet.packet_id = packet_id;
        packet.tensor_id = tensor_id;
        packet.packet_size = packet_size;
        packet.addr = (uint64_t*)malloc(16*sizeof(uint64_t));
        packet.req_type = (int*)malloc(16*sizeof(uint32_t));
        packet.flit_word_num = (int*)malloc(16*sizeof(uint32_t));
        packet.data = NEW2D(16, 8, int);
        for(int i=0;i<packet_size;i++){
            packet.addr[i] = GETADDR(ptr, i);
            packet.req_type[i] = GETREQ_TYPE(ptr, i);
            packet.flit_word_num[i] = GETFLIT_WORD_NUM(ptr, i);
            for(int j=0;j<packet.flit_word_num[i];j++){
                packet.data[i][j] = GETDATA(ptr, i, j);
            }
        }
        setIPC_Ack(ptr);
        transPacketToTraffic(packet, id);
        while(CHECKACK(ptr)!=0){
        }
        pthread_mutex_unlock(&mutex[id]);
        //* Check dependcy table
        for(int i=0;i<NIC_NUM;i++)
            pthread_mutex_lock(&mutex[i]);
        if(checkDependencyTable(tensor_id, packet_num, id)){
            noxim_finish = true;
            printf("Noxim input finished!\n");
        }
        for(int i=0;i<NIC_NUM;i++)
            pthread_mutex_unlock(&mutex[i]);
    }
}

void* sentBackToNoC(void* arg){
    int* cache_nic_id = (int*)arg;
    int id = (*cache_nic_id);
    int nic_id = simple_log_2(PE_NUM)*(id+1) + id;
    while(!noc_finish[id]){
        int front = trafficTableQueue[id]->front;
        if(trafficTableQueue[id]->trafficTable[front].dst_id == nic_id && trafficTableQueue[id]->trafficTable[front].finished){
            //* Verify traffic id if req is in same packet, since CacheSim may run in out of order.
            pthread_mutex_lock(&mutex[id]);
            traffic_t traffic;
            dequeue(&traffic, trafficTableQueue[id]);
            pthread_mutex_unlock(&mutex[id]);
            int validIndex = -1;
            pthread_mutex_lock(&mutex_sendBack);
            for(int i=0;i<SEND_BACK_PACKET_SIZE;i++){
                if(validIndex == -1 && sendBackPacket[i].packet_id == -1){
                    // Setting valid index in case that there is no existed packet in sendBackPacket
                    validIndex = i;
                }
                if(traffic.packet_id == sendBackPacket[i].packet_id){
                    // The packet is already existed in sendBackPacket
                    validIndex = i;
                    break;
                }
            }
            if(!traffic.tail){
                int index = traffic.sequence_no;
                sendBackPacket[validIndex].packet_id = traffic.packet_id;
                sendBackPacket[validIndex].req_type[index] = traffic.req_type;
                sendBackPacket[validIndex].flit_word_num[index] = traffic.flit_word_num;
                sendBackPacket[validIndex].addr[index] = traffic.addr;
                for(int i=0;i<traffic.flit_word_num;i++)
                    sendBackPacket[validIndex].data[index][i] = traffic.data[i];
                pthread_mutex_unlock(&mutex_sendBack);
            }
            else{
                int index = traffic.sequence_no;
                sendBackPacket[validIndex].src_id = traffic.src_id;
                sendBackPacket[validIndex].dst_id = traffic.dst_id;
                sendBackPacket[validIndex].packet_size = traffic.packet_size;
                sendBackPacket[validIndex].packet_num = traffic.packet_num;
                sendBackPacket[validIndex].packet_id = traffic.packet_id;
                sendBackPacket[validIndex].tensor_id = traffic.tensor_id;
                sendBackPacket[validIndex].req_type[index] = traffic.req_type;
                sendBackPacket[validIndex].flit_word_num[index] = traffic.flit_word_num;
                sendBackPacket[validIndex].addr[index] = traffic.addr;
                for(int i=0;i<traffic.flit_word_num;i++)
                    sendBackPacket[validIndex].data[index][i] = traffic.data[i];
                Packet packet;
                assignPacket(&packet, sendBackPacket[validIndex]);
                resetSendBackPacket(&sendBackPacket[validIndex]);
                pthread_mutex_unlock(&mutex_sendBack);
                const char* id_name;
                switch(id){
                    case 0:
                        id_name = "0_CACHE";
                        break;
                    case 1:
                        id_name = "1_CACHE";
                        break;
                    case 2:
                        id_name = "2_CACHE";
                        break;
                    case 3:
                        id_name = "3_CACHE";
                        break;
                    default:
                        id_name = "0_CACHE";
                        break;
                }
                const char* shm_name = concatenateStrings(SHM_NAME, id_name);
                //* Creating shared memory object
                int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
                //* Setting file size
                ftruncate(fd, SHM_SIZE);
                //* Mapping shared memory object to process address space
                uint32_t* ptr = (uint32_t*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                uint32_t ready, valid, ack;
                memset(ptr, 0, SHM_SIZE);
                while(CHECKREADY(ptr) != 1){
                }
                //* set src_id (switch)
                setIPC_Data(ptr, packet.dst_id, 1, 0);
                //* set dst_id (switch)
                setIPC_Data(ptr, packet.src_id, 2, 0);
                // set packet_id
                setIPC_Data(ptr, packet.packet_id, 3, 0);
                // set packet_num
                setIPC_Data(ptr, packet.packet_num, 4, 0);
                // set tensor_id
                setIPC_Data(ptr, packet.tensor_id, 5, 0);
                // set packet size
                setIPC_Data(ptr, packet.packet_size, 6, 0);
                for(int i=0;i<packet.packet_size;i++){
                    // set request addr
                    setIPC_Addr(ptr, packet.addr[i], i);
                    // set request type
                    setIPC_Data(ptr, packet.req_type[i], 8, i);
                    // set flit_word_num
                    setIPC_Data(ptr, packet.flit_word_num[i], 9, i);
                    for(int j=0;j<packet.flit_word_num[i];j++){
                        // set request data
                        setIPC_Data(ptr, packet.data[i][j], 10+j, i);
                    }
                }
                setIPC_Valid(ptr);
                sendback_count[id]++;
                while(CHECKACK(ptr) != 1){
                }
                resetIPC_Valid(ptr);
                resetIPC_Ack(ptr);
            }
            //* Check if there is any request to be sent back when Cache Simulator finishs all its work
            //* Otherwise, set finish signal for this Cache NIC IPC channel
            if(cache_finish){
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
            int input_status;
            // * Load input to request queue
            for(int i =0;i<traffic_queue_num;i++){
                pthread_mutex_lock(&mutex[i]);
                input_status = checkTrafficTable(trafficTableQueue[i], i);
                pthread_mutex_unlock(&mutex[i]);
                if(input_status == 0){
                    // Traffic Table i is empty
                }
                else if(input_status == 2){
                    // Request Queue of Traffic Table i is full
                    stall_RequestQueue++;
                    break;
                }
                else if(input_status == 3){
                    // No request of Traffic Table i can be processed now
                    break;
                }
                else{
                    // No traffic from Traffic Table i can be processed now
                }
            }
            // * Check the MSHR & MAF status for each bank
            for(int j=0;j<cache->bank_size;j++){
                // Only need to check MSHR status when MSHR enabled
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
                            cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, addr_mode, cache->cache_bank[j].mshr_queue->mshr[i].maf[0].req_number_on_trace);
                        }
                    }
                }
                // * When request queue is not empty, send request to cache bank
                if(!req_queue_empty(cache->cache_bank[j].request_queue)){
                    req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
                }
                else{
                    // Request queue of bank %d is empty
                }
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
            for(int i=0;i<NIC_NUM;i++)
                pthread_mutex_lock(&mutex[i]);
            printf("\n-------------------------\n");
            printf("Traffic Table is empty.\n");
            printf("Cycle: %lld\n", cycles);
            printf("\n-------------------------\n");
            getchar();
            for(int i=0;i<NIC_NUM;i++)
                pthread_mutex_unlock(&mutex[i]);
        }
    }
}

void executeRemainTraffic(){
    //! Execution not done yet => request queue need to be cleared
    printf("Clearing Request Queue & MSHR Queue...\n");
    printf("Cycle: %lld\n", cycles);
    printf("=====================\n");
    // Print the state of queue in each bank
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
    // MSHR enabled => need to check if MSHR is cleared(stalled) & request queue is cleared
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
              cacheset_load_MSHR_data(cache->cache_bank[j].set_num, j, cache, cache->cache_bank[j].cache_set, cache->cache_bank[j].mshr_queue->mshr[i].addr, mshr_queue_clear_inst(cache->cache_bank[j].mshr_queue,i), &writebacks, &non_dirty_replaced, addr_mode, cache->cache_bank[j].mshr_queue->mshr[i].maf[0].req_number_on_trace);
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
        cycles++;
        for(int j = 0;j<cache->bank_size;j++){
          if(!req_queue_empty(cache->cache_bank[j].request_queue)){
            req_send_to_set(cache, cache->cache_bank[j].request_queue, cache->cache_bank, j);
          }
          else if(cache->cache_bank[j].stall){
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

bool checkDependencyTable(int tensor_id, int packet_num, int nic_id){
    for(int i=0;i<FIN_TENSOR_NUM;i++){
        if(tensorDependencyTable[i][nic_id].tensor_id == tensor_id){
            if(tensorDependencyTable[i][nic_id].return_flag){
                fprintf(stderr, "(Error) The return flag of tensor id %d is already pull up, but still get same tensor id packet\n", tensor_id);
                exit(EXIT_FAILURE);
            }
            else{
                if(tensorDependencyTable[i][nic_id].packet_count == -1){
                    printf("(NIC%d) Tensor id %d packet num = %d\n", nic_id, tensor_id, packet_num);
                    tensorDependencyTable[i][nic_id].packet_count = packet_num - 1;
                    printf("(NIC%d) Now is waiting for %d packets\n", nic_id, tensorDependencyTable[i][nic_id].packet_count);
                    return false;
                }
                else{
                    tensorDependencyTable[i][nic_id].packet_count = tensorDependencyTable[i][nic_id].packet_count - 1;
                    if(tensorDependencyTable[i][nic_id].packet_count == 0){
                        tensorDependencyTable[i][nic_id].return_flag = true;
                        int* flag = (int*) malloc(nic_id*sizeof(int));
                        printf("(NIC%d) Tensor id %d is finished!\n", nic_id, tensor_id);
                        printf("-----------\n");
                        //* To ensure every packet from different NIC will be logged on DependencyTable
                        //* If table is labeled as return, means one of the NIC complete this finish tensor
                        //* If talbe packet count is -1, means this nic entry did not sent this tensor
                        //* If any of NIC is running this tensor, flag will be marked as 0 and break loop
                        //! Only when more than one NIC already finish this tensor, AND no other NIC is sending this tensor. Then this tensor will be marked as finish input
                        for(int j=0;j<FIN_TENSOR_NUM;j++){
                            for(int k=0;k<NIC_NUM;k++){
                                if(tensorDependencyTable[j][k].return_flag)
                                    flag[j] += NIC_NUM;
                                else if(tensorDependencyTable[j][k].packet_count==-1)
                                    flag[j]--;
                                else if(!tensorDependencyTable[j][k].return_flag && tensorDependencyTable[j][k].packet_count!=-1){
                                    flag[j] = 0;
                                    break;
                                }
                            }
                        }
                        bool fin_flag = true;
                        for(int j=0;j<FIN_TENSOR_NUM;j++)
                            if(flag[j]<=0)
                                fin_flag = false;
                        if(fin_flag){
                            printf("==========\n");
                            printf("All tensor is finished!\n");
                            printf("==========\n");
                            return true;
                        }
                    }
                    return false;
                }
            }
        }
    }
    return false;
}

void transPacketToTraffic(Packet packet, int id){
    bool loop = true;
    int packet_size = packet.packet_size;
    int counter = 0;
    while(loop){
        traffic_t traffic;
        traffic.src_id = packet.src_id;
        traffic.dst_id = packet.dst_id;
        traffic.packet_id = packet.packet_id;
        traffic.sequence_no = counter;
        traffic.tensor_id = packet.tensor_id;
        traffic.packet_size = packet.packet_size;
        traffic.packet_num = packet.packet_num;
        traffic.addr = packet.addr[counter];
        traffic.req_type = packet.req_type[counter];
        traffic.flit_word_num = packet.flit_word_num[counter];
        traffic.data = (int*)malloc(MAX_FLIT_WORDSIZE*sizeof(int));
        for(int i=0;i<traffic.flit_word_num;i++){
            traffic.data[i] = packet.data[counter][i];
        }
        traffic.valid = true;
        traffic.working = false;
        traffic.finished = false;
        traffic.tail = false;
        counter++;
        if(counter >= packet_size){
            loop = false;
            traffic.tail = true;
        }
        enqueue(trafficTableQueue[id], traffic);
    }
}

void setIPC_Data(uint32_t *ptr, int data, int const_pos, int index){
    *(ptr + const_pos + 12*index) = data;
    return;
}

void setIPC_Addr(uint32_t *ptr, uint64_t data, int index){
    uint64_t *dataPtr = reinterpret_cast<uint64_t*>(ptr + 7 + 12 * index);
    *dataPtr = data;
    return;
}

void setIPC_Ready(uint32_t *ptr){
    *ptr = (*ptr | (0b1 << 31));
    return;
}

void resetIPC_Ready(uint32_t *ptr){
    *ptr = (*ptr & ~(0b1 << 31));
    return;
}

void setIPC_Valid(uint32_t *ptr){
    *ptr = (*ptr | (0b1 << 30));
    return;
}

void resetIPC_Valid(uint32_t *ptr){
    *ptr = (*ptr & ~(0b1 << 30));
    return;
}

void setIPC_Ack(uint32_t *ptr){
    *ptr = (*ptr | (0b1 << 29));
    return;
}

void resetIPC_Ack(uint32_t *ptr){
    *ptr = (*ptr & ~(0b1 << 29));
    return;
}

void assignPacket(Packet* p, Packet assign_packet){
    p->src_id = assign_packet.src_id;
    p->dst_id = assign_packet.dst_id;
    p->packet_size = assign_packet.packet_size;
    p->packet_num = assign_packet.packet_num;
    p->packet_id = assign_packet.packet_id;
    p->tensor_id = assign_packet.tensor_id;
    p->addr = (uint64_t*) malloc(p->packet_size*sizeof(uint64_t));
    p->req_type = (int*) malloc(p->packet_size*sizeof(int));
    p->flit_word_num = (int*) malloc(p->packet_size*sizeof(int));
    p->data = NEW2D(p->packet_size, MAX_FLIT_WORDSIZE, int);
    for(int i=0;i<p->packet_size;i++){
        p->addr[i] = assign_packet.addr[i];
        p->req_type[i] = assign_packet.req_type[i];
        p->flit_word_num[i] = assign_packet.flit_word_num[i];
        for(int j=0;j<p->flit_word_num[i];j++){
            p->data[i][j] = assign_packet.data[i][j];
        }
    }
}

void resetSendBackPacket(Packet* packet){
    packet->src_id = 0;
    packet->dst_id = 0;
    packet->packet_size = 0;
    packet->packet_num = 0;
    packet->packet_id = -1;
    packet->tensor_id = 0;
    for(int i=0;i<MAX_PACKET_FLITNUM;i++){
        packet->req_type[i] = 0;
        packet->flit_word_num[i] = 0;
        packet->addr[i] = 0;
        for(int j=0;j<MAX_FLIT_WORDSIZE;j++){
            packet->data[i][j] = 0;
        }
    }
}

void sendBackPacket_init(){
    sendBackPacket = (Packet*)malloc(SEND_BACK_PACKET_SIZE*sizeof(Packet));
    for(int i=0;i<SEND_BACK_PACKET_SIZE;i++){
        sendBackPacket[i].src_id = 0;
        sendBackPacket[i].dst_id = 0;
        sendBackPacket[i].packet_size = 0;
        sendBackPacket[i].packet_num = 0;
        sendBackPacket[i].packet_id = -1;
        sendBackPacket[i].tensor_id = 0;
        sendBackPacket[i].addr = (uint64_t*) malloc(MAX_PACKET_FLITNUM*sizeof(uint64_t));
        sendBackPacket[i].req_type = (int*) malloc(MAX_PACKET_FLITNUM*sizeof(int));
        sendBackPacket[i].flit_word_num = (int*) malloc(MAX_PACKET_FLITNUM*sizeof(int));
        sendBackPacket[i].data = NEW2D(MAX_PACKET_FLITNUM, MAX_FLIT_WORDSIZE, int);
    }
}