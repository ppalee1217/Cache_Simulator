#ifndef INPUT_GEN_H
#define INPUT_GEN_H
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>

void generate_mapping1(FILE *f, int bank_num, int test_num);
void generate_mapping0(FILE *f);
void generate_random_test(FILE *f, int test_num);

#endif