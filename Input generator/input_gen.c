#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include "input_gen.h"

int main() {
  FILE *f = fopen("trace.txt", "w");
  if(f == NULL){
    printf("Error opening txt file\n");
    exit(1);
  }
  // generate_mapping0(f, 4, 48, 256, true);
  // generate_susume(f, 4, 48, 256, false);
  // generate_susume(f, 4, 48, 256, false);
  // generate_susume(f, 4, 48, 256, false);
  // generate_susume(f, 4, 48, 256, false);
  generate_random_test(f, 1000);
  fclose(f);
  return 0;
}

// Spatial locality only (The data will never repeat)
//! Byte base
void generate_susume(FILE *f, int bank_num, int test_num, int set_num, bool same_index){
  // time_t t;
  // srand((unsigned int) time(&t));
  // unsigned int x = rand();
  unsigned int x = rand();
  for (int i=0;i < 500;i++){
    fprintf(f,"1 %08x 0\n", x);
    x += 4;
  }
}

// Temporal locality only


// Spatial & Temporal locality (Multiple loops that will repeat several times)
//* Note that distance between every read data addr is 4 bytes (1 word = 1 block)

void generate_random_test(FILE *f, int test_num){
  time_t t;
  srand((unsigned int) time(&t));
  for(int i=0; i<test_num; i++){
    unsigned int x = rand();
    if (x%2 == 0)
      x= x | (1<<31);
    fprintf(f,"1 %08x 0\n",x);
  }
}

void generate_mapping0(FILE * f, int bank_num, int test_num, int set_num, bool same_index) {
  unsigned int same_index_num = 0;
  bool same_index_flag = same_index;
  time_t t;
  srand((unsigned int) time(&t));
  unsigned int temp;
  int choose_bank = 0;
  int index_mask = 0;
  for(int i=0;i<log2(set_num);i++){
    index_mask = (index_mask << 1) + 1;
  }
  for(int i=0; i<test_num; i++){
    if(i >= (test_num/bank_num)*(choose_bank+1)){
      choose_bank++;
      printf("test_num: %d\n",i);
      printf("Choose bank %d\n",choose_bank);
      same_index_flag = true;
    }
    unsigned int x = rand();
    unsigned int index = (x >> 5) & index_mask;
    while(((index - choose_bank) % bank_num != 0) || (index != same_index_num && same_index)){
      x = rand();
      index = (x >> 5) & index_mask;
      if(same_index && same_index_flag){
        if((index - choose_bank) % bank_num == 0){
          same_index_num = index;
          same_index_flag = false;
        } 
      }
      if(same_index){
        unsigned int temp2 = (x >> (int)(5 + log2(set_num)));
        temp2 <<= (int)log2(set_num);
        temp2 += same_index_num;
        temp2 <<= 5;
        temp2 += (unsigned int)(0b11111 & x);
        x = temp2;
        index = (x >> 5) & index_mask;
      }
    }
    if(same_index && same_index_flag){
      if((index - choose_bank) % bank_num == 0){
        same_index_num = index;
        same_index_flag = false;
      } 
    }
    printf("i = %d\n",i);
    printf("x = %x\n",x);
    printf("index: %d\n",index);
    // printf("choose_bank: %d\n",choose_bank);
    fprintf(f,"1 %08x 0\n",x);
  }
}

void generate_mapping1(FILE * f, int bank_num, int test_num, int set_num, bool same_index) {
  time_t t;
  srand((unsigned int) time(&t));
  unsigned int temp;
  unsigned int address_partition = (unsigned int)-1 / bank_num;
  int choose_bank = 0;
  int index_mask = 0;
  for(int i=0;i<log2(set_num);i++){
    index_mask = (index_mask << 1) + 1;
  }
  for(int i=0; i<test_num; i++){
    unsigned int x = rand();
    //! make index < 10
    while(((x & (index_mask<<3))>>3) >10){
      x = rand();
    }
    while(x/address_partition != (choose_bank)){
      unsigned int temp = address_partition * choose_bank;
      x = rand();
      x += temp;
      while(((x & (index_mask<<3))>>3) >10){
        x = rand();
      }
    }
    if(i > (test_num/bank_num)*(choose_bank+1)){
      choose_bank++;
      printf("test_num: %d\n",i);
      printf("Choose bank %d\n",choose_bank);
    }
    fprintf(f,"1 %08x 0\n",x);
  }
}