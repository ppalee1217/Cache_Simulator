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
  // generate_mapping1(f,4,80000);
  generate_random_test(f,5000);
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

void generate_mapping1(FILE * f,int bank_num, int test_num) {
  time_t t;
  srand((unsigned int) time(&t));
  unsigned int temp;
  unsigned int address_partition = (unsigned int)-1 / bank_num;
  int choose_bank = 0;
  for(int i=0; i<test_num; i++){
    unsigned int x = rand();
    while(((x & (0xff<<3))>>3) >10){
      x = rand();
    }
    while(x/address_partition != (choose_bank)){
      unsigned int temp = address_partition * choose_bank;
      x = rand();
      x += temp;
      while(((x & (0xff<<3))>>3) >10){
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

void generate_mapping0(FILE * f) {
  // Random
  time_t t;
  srand((unsigned int) time(&t));
  unsigned int temp;
  for(int i=0;i<40000;i++){
    unsigned int x = rand();
    while(((x & (0xff<<3))>>3) >10){
      x = rand();
    }
    if(i < 5000){
      fprintf(f,"1 %08x 0\n",x >> 4);
      temp = x >> 4;
    }
    else if(i < 10000){
      // temp += 32;
      fprintf(f,"1 %08x 0\n",temp);
    }
    else if(i < 15000){
      fprintf(f,"1 %x 0\n",(x >> 4) | ((unsigned int)0x1 << 30));
      temp = (x >> 4) | ((unsigned int)0x1 << 30);
    }
    else if(i < 20000){
      // temp += 32;
      fprintf(f,"1 %08x 0\n",temp);
    }
    else if(i < 25000){
      fprintf(f,"1 %x 0\n",(x >> 4) | ((unsigned int)0x2 << 30));
      temp = (x >> 4) | ((unsigned int)0x2 << 30);
    }
    else if(i < 30000){
      // temp += 32;
      fprintf(f,"1 %08x 0\n",temp);
    }
    else if(i < 35000){
      fprintf(f,"1 %x 0\n",(x >> 4) | ((unsigned int)0x3 << 30));
      temp = (x >> 4) | ((unsigned int)0x3 << 30);
    }
    else{
      // temp += 32;
      fprintf(f,"1 %08x 0\n",temp);
    }
  }
}