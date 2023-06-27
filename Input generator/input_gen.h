#ifndef INPUT_GEN_H
#define INPUT_GEN_H
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <stdbool.h>

void generate_mapping0(FILE * f, int bank_num, int test_num, int set_num, bool same_index);
void generate_mapping1(FILE *f, int bank_num, int test_num, int set_num, bool same_index);
void generate_random_test(FILE *f, int test_num);
void generate_susume(FILE *f, int bank_num, int test_num, int set_num, bool same_index);
#endif