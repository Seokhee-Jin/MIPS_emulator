//
// Created by 진석희 on 2023/04/04.
//


#include "instruction.h"

#ifndef SIMPLECALCULATOR_STRING_TO_INSTRUCTION_H
#define SIMPLECALCULATOR_STRING_TO_INSTRUCTION_H

#define MAX_LINE_LEN (20)
#define MAX_FILE_LEN (1024)

int count_lines(const char* filename);
Instruction** get_instructions(char** lines, int num_lines);
Instruction** file_to_instructions(char* filename);

int str_to_constant(const char* str);
int str_to_register_index(const char* str);

#endif //SIMPLECALCULATOR_STRING_TO_INSTRUCTION_H
