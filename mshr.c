#include "mshr.h"

/**
 * Function to initialize an MSHR Queue for a cache bank with a given <entries>. This function
 * creates the MSHR Queue. 
 * 
 * @param entires is the size of the MSHR Queue to initialize.
 * @param inst_num is the size of the MAF to initialize.
 * @return the dynamically allocated queue. 
 */
mshr_queue_t* init_mshr_queue(int bank_num, int entires, int inst_num) {
  //  Use malloc to dynamically allocate a mshr_queue_t
	mshr_queue_t* queue = (mshr_queue_t*) malloc(sizeof(mshr_queue_t));
  if(entires == 0){
    queue->enable_mshr = false;
  }
  else{
    queue->enable_mshr = true;
  }
  queue->entries = entires;
  queue->bank_num = bank_num;
  queue->mshr = init_mshr(entires ,inst_num);
  return queue;
}

/**
 * Function to initialize an single MSHR for a MSHR Queue with a given <inst_num>. This function
 * creates a MSHR. 
 * 
 * @param inst_num is the size of the MAF to initialize.
 * @return the dynamically allocated MSHR. 
 */
mshr_t* init_mshr(int entires, int inst_num){
  mshr_t* mshr = (mshr_t*) malloc(sizeof(mshr_t)*entires);
  if(entires != 0){
    for(int j=0;j<entires;j++){
      mshr[j].valid = false;
      mshr[j].issued = false;
      mshr[j].maf_size = inst_num;
      mshr[j].maf = (maf_t*) malloc(sizeof(maf_t)*inst_num);
      mshr[j].maf_used_num = 0;
      mshr[j].last_index = -1;
      for(int i = 0; i<inst_num;i++){
        mshr[j].maf[i].valid = false;
      }
    }
  }
  return mshr;
}

/**
 * Log the new request info issued to the DRAM
 * 
 * @param type is the operation type of the instruction - 0 for load, 1 for store, 2 for instruction read (not used).
 * @return -1 if the MAF is full, 0 if the MSHR is full, 1 if the request is not in MSHR, 2 if the request is in MSHR and MAF is not full.
 */
int mshr_queue_get_entry(mshr_queue_t* queue, unsigned long long addr, unsigned int type, unsigned int block_offset, unsigned int destination, int tag, int req_number_on_trace, int index, traffic_t* traffic){
  if(queue->enable_mshr){
    int empty_entry = -1;
    //* Search if there exist MSHR entry has isseued to the same tag
    for(int i =0 ;i<queue->entries;i++){
      //! This MSHR entry is already exist
      if(queue->mshr[i].valid && (queue->mshr[i].tag) == (tag) && (queue->mshr[i].index == index)){
        // printf("This tag MSHR entry is already exist\n");
        for(int j = 0; j<queue->mshr[i].maf_size;j++){
          if(!queue->mshr[i].maf[j].valid){
            //! MAF is not full
            // printf("And MAF is not full\n");
            queue->mshr[i].maf[j].valid = true;
            queue->mshr[i].maf[j].type = type;
            queue->mshr[i].maf[j].req_number_on_trace = req_number_on_trace;
            queue->mshr[i].maf_used_num++;
            //* Trace traffic of noxim
            queue->mshr[i].maf[j].traffic = traffic;
            return 2;
          }
        }
        //* If MAF queue is full, return -1 (to stall the pipeline)
        // printf("But MAF is full\n");
        return -1;
      }
    }
    //* No MSHR entry of same tag exist, search for empty entry
    for(int i=0;i<queue->entries;i++){
      //! Always tend to find an empty MSHR entry has lower index
      if(!queue->mshr[i].valid){
        empty_entry = i;
        break;
      }
    }
    //! Check if there is any empty entry left
    if(empty_entry != -1){
        // printf("There is empty entry at %d of MSHR\n", empty_entry);
        queue->mshr[empty_entry].valid = true;
        queue->mshr[empty_entry].issued = false;
        queue->mshr[empty_entry].addr = addr;
        // printf("\n\n\n\nMSHR logged addr %016llx\n\n\n\n", addr);
        // getchar();
        queue->mshr[empty_entry].tag = tag;
        queue->mshr[empty_entry].index = index;
        queue->mshr[empty_entry].maf[0].valid = true;
        queue->mshr[empty_entry].maf[0].type = type;
        queue->mshr[empty_entry].maf[0].req_number_on_trace = req_number_on_trace;
        queue->mshr[empty_entry].maf_used_num++;
        //* Trace traffic of noxim
        queue->mshr[empty_entry].maf[0].traffic = traffic;
        return 1;
    }
    //* There is no empty MSHR entry left, return false (to stall the pipeline)
    // printf("There is no empty entry\n");
    return 0;
  }
}

int mshr_queue_check_isssue(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    if(queue->mshr[i].valid && !queue->mshr[i].issued){
      queue->mshr[i].issued = true;
      return i;
    }
  }
  return -1;
}

void mshr_queue_check_data_returned(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    if(queue->mshr[i].valid && queue->mshr[i].issued){
      if(queue->mshr[i].counter == miss_cycle){
        queue->mshr[i].counter = 0;
        queue->mshr[i].data_returned = true;
      }
    }
  }
}

int mshr_queue_check_exist(mshr_queue_t* queue, unsigned int tag, int index){
  //* Check if the tag is already in the MSHR
  //* If it is, return the index of the MSHR
  //* Else, return -1 
  for(int i =0 ;i<queue->entries;i++){
    if(queue->mshr[i].valid && (queue->mshr[i].tag == tag) && (queue->mshr[i].index == index)){
      return i;
    }
  }
  return -1;
}

void mshr_queue_counter_add(mshr_queue_t* queue){
  for(int i =0 ;i<queue->entries;i++){
    if(queue->mshr[i].valid && queue->mshr[i].issued){
      queue->mshr[i].counter++;
    }
  }
}

int mshr_queue_clear_inst(mshr_queue_t* queue, int entries){
  if(queue->mshr[entries].valid && queue->mshr[entries].data_returned){
    if(queue->mshr[entries].last_index != -1){
      int index = queue->mshr[entries].last_index + 1;
      if(index >= queue->mshr[entries].maf_size || !queue->mshr[entries].maf[index].valid){
        index = 0;
      }
      queue->mshr[entries].maf[index].valid = false;
      queue->mshr[entries].maf_used_num--;
      queue->mshr[entries].last_index = index;
      //* Trace traffic of noxim
      if(running_mode == 2){
        queue->mshr[entries].maf[index].traffic->finished = true;
        // if(queue->mshr[entries].maf[index].traffic->tensor_id==2 || queue->mshr[entries].maf[index].traffic->tensor_id==5 || queue->mshr[entries].maf[index].traffic->tensor_id==8 || queue->mshr[entries].maf[index].traffic->tensor_id==11)
        //     printf("(3)tensor id %d is finished\n", queue->mshr[entries].maf[index].traffic->tensor_id);
        // printf("(3)Traffic packet id %d finished, flit wd num = %d\n", queue->mshr[entries].maf[index].traffic->packet_id, queue->mshr[entries].maf[index].traffic->flit_word_num);
        if(queue->mshr[entries].maf[index].traffic->req_type)
            for(int i =0;i<queue->mshr[entries].maf[index].traffic->flit_word_num;i++)
                queue->mshr[entries].maf[index].traffic->data[i] = READ_DATA;
      }
      if(queue->mshr[entries].maf_used_num==0){
        queue->mshr[entries].valid = false;
        queue->mshr[entries].issued = false;
        queue->mshr[entries].data_returned = false;
      }
      queue->mshr[entries].last_index = index;
      return queue->mshr[entries].maf[index].type;
    }
    else{
      for(int j = 0; j<queue->mshr[entries].maf_size;j++){
        if(queue->mshr[entries].maf[j].valid){
          // keep looping to clear maf inst
          queue->mshr[entries].maf[j].valid = false;
          queue->mshr[entries].maf_used_num--;
          //* Trace traffic of noxim
          if(running_mode == 2){
            queue->mshr[entries].maf[j].traffic->finished = true;
            // if(queue->mshr[entries].maf[j].traffic->tensor_id==2 || queue->mshr[entries].maf[j].traffic->tensor_id==5 || queue->mshr[entries].maf[j].traffic->tensor_id==8 || queue->mshr[entries].maf[j].traffic->tensor_id==11)
            //     printf("(4)tensor id %d is finished\n", queue->mshr[entries].maf[j].traffic->tensor_id);
            // printf("(4)Traffic packet id %d finished\n", queue->mshr[entries].maf[j].traffic->packet_id);
            if(queue->mshr[entries].maf[j].traffic->req_type)
                for(int i =0;i<queue->mshr[entries].maf[j].traffic->flit_word_num;i++)
                    queue->mshr[entries].maf[j].traffic->data[i] = READ_DATA;
          }
          if(queue->mshr[entries].maf_used_num==0){
            queue->mshr[entries].valid = false;
            queue->mshr[entries].issued = false;
            queue->mshr[entries].data_returned = false;
          }
          queue->mshr[entries].last_index = j;
          return queue->mshr[entries].maf[j].type;
        }
      }
    }
  }
  return -1;
}

//* Return the logged MAF entry index
int log_maf_queue(int mshr_index, mshr_queue_t* queue, unsigned long long addr, unsigned int type, unsigned int block_offset, unsigned int destination, int tag, traffic_t* traffic){
  for(int j = 0; j<queue->mshr[mshr_index].maf_size;j++){
    if(!queue->mshr[mshr_index].maf[j].valid){
      //! MAF is not full
      queue->mshr[mshr_index].maf[j].valid = true;
      queue->mshr[mshr_index].maf[j].type = type;
      queue->mshr[mshr_index].maf_used_num++;
      //* Trace traffic of noxim
      queue->mshr[mshr_index].maf[j].traffic = traffic;
      return j;
    }
  }
}

void mshr_queue_cleanup(mshr_queue_t* queue) {
  for(int i = 0; i<queue->entries;i++){
    free(queue->mshr[i].maf);
  }
  free(queue->mshr);
  free(queue);
}