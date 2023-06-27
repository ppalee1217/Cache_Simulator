#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main() {
  int counter = 0;
  printf("Program to compare the output of the two cache simulators\n");
  FILE *wb1 = fopen("wb1.txt", "r");
  if(wb1 == NULL){
    printf("Error opening txt file\n");
    exit(1);
  }
  FILE *wb2 = fopen("wb2.txt", "r");
  if(wb2 == NULL){
    printf("Error opening txt file\n");
    exit(1);
  }
  while(!feof(wb1) && !feof(wb2)){
    counter++;
    char* addr1 = (char*) malloc(sizeof(char)*100);
    char* addr2 = (char*) malloc(sizeof(char)*100);
    fgets(addr1, 100, wb1);
    fgets(addr2, 100, wb2);
    if(strcmp(addr1, addr2) != 0){
      printf("addr1 %s", addr1);
      printf("addr2 %s", addr2);
      printf("Error occurs at line %d\n", counter);
      exit(1);
    }
  }
  fclose(wb1);
  fclose(wb2);
  return 0;
}