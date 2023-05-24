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
  generate_mapping0(f, 4, 12, 256);
  fclose(f);
  return 0;
}

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

void generate_mapping0(FILE * f, int bank_num, int test_num, int set_num) {
  time_t t;
  srand((unsigned int) time(&t));
  unsigned int temp;
  int choose_bank = 0;
  int index_mask = 0;
  for(int i=0;i<log2(set_num);i++){
    index_mask = (index_mask << 1) + 1;
  }
  for(int i=0; i<test_num; i++){
    unsigned int x = rand();
    int index = (x >> (int)(log2(set_num) + 2)) & index_mask;
    while((index - choose_bank) % bank_num != 0){
      x = rand();
      index = (x >> (int)(log2(set_num) + 2)) & index_mask;
    }
    printf("i = %d\n",i);
    printf("index: %d\n",index);
    printf("choose_bank: %d\n",choose_bank);
    if(i >= (test_num/bank_num)*(choose_bank+1)){
      choose_bank++;
      printf("test_num: %d\n",i);
      printf("Choose bank %d\n",choose_bank);
    }
    fprintf(f,"1 %08x 0\n",x);
  }
}

void generate_mapping1(FILE * f, int bank_num, int test_num, int set_num) {
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