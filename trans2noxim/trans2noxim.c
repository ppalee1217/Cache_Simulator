#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <math.h>
#include <string.h>

int main() {
  int counter = 0;
  printf("Program to transform the input trace of Cache Simulator to NOXIM\n");
  FILE *trace_cache = fopen("trace_cache.txt", "r");
  if(trace_cache == NULL){
    printf("Error opening txt file\n");
    exit(1);
  }
  FILE *trace_noxim = fopen("t.txt", "w");
  if(trace_noxim == NULL){
    printf("Error opening txt file\n");
    exit(1);
  }
  time_t t;
  srand((unsigned int) time(&t));
  while(!feof(trace_cache)){
    int type, finish;
    unsigned long long address;
    unsigned int* data = (unsigned int*)malloc(sizeof(unsigned int)*8);
    unsigned int src_id;
    fscanf(trace_cache, "%d %llx %x %x %x %x %x %x %x %x\n", &type, &address, &data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6], &data[7]);
    src_id = rand()%20;
    while(src_id == 0 || (src_id+1)%5 == 0){
        src_id = rand()%20;
    }
    int req_size = 8;
    // src, dst, finish, count, isReqt, req_size, req_addr,
    // req_type, req_data[0], req_data[1], req_data[2], req_data[3],
    // req_data[4], req_data[5], req_data[6], req_data[7],
    // pir, por, t_on, t_off, t_period
    if(feof(trace_cache)){
        fprintf(trace_noxim,"%d %d %d %d %d %d %016llx %d %08x %08x %08x %08x %08x %08x %08x %08x\n",src_id, -1, 1, 1, 1, req_size, address, type, 0, 0, 0, 0, 0, 0, 0, 0);
        fprintf(trace_noxim,"%d %d %d %d %d %d %016llx %d %08x %08x %08x %08x %08x %08x %08x %08x\n",src_id, -1, 1, 1, 0, req_size, address, type, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    }
    else{
        fprintf(trace_noxim,"%d %d %d %d %d %d %016llx %d %08x %08x %08x %08x %08x %08x %08x %08x\n",src_id, -1, 0, 1, 1, req_size, address, type, 0, 0, 0, 0, 0, 0, 0, 0);
        fprintf(trace_noxim,"%d %d %d %d %d %d %016llx %d %08x %08x %08x %08x %08x %08x %08x %08x\n",src_id, -1, 0, 1, 0, req_size, address, type, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    }
  }
  printf("Transformation Done!\n");
  fclose(trace_cache);
  fclose(trace_noxim);
  return 0;
}