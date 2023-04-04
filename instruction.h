//
// Created by 진석희 on 2023/04/03.
//
#include <stdbool.h>

#ifndef SIMPLECALCULATOR_INSTRUCTION_H
#define SIMPLECALCULATOR_INSTRUCTION_H


#define NUM_REGISTERS (10)

typedef struct Instruction {
    char* opcode;
    char* operand1;
    char* operand2;
    char* operand3;
} Instruction;

typedef struct Register {
    int register_val_array[NUM_REGISTERS]; // 레지스터에 저장될 값
    bool register_status_array[NUM_REGISTERS]; // 레지스터 사용 여부
} Register;

void print_register(Register* reg_p);
void print_instruction(Instruction* instruction);
int run_instruction(Instruction* ins_p, Register* reg_p);

int add(Instruction*, Register*);
int sub(Instruction*, Register*);
int mul(Instruction*, Register*);
int div_(Instruction*, Register*);
int mov(Instruction*, Register*);
int lw(Instruction*, Register*);
int sw(Instruction*, Register*);
int rst(Instruction*, Register*);
int jmp(Instruction*, Register*);
int beq(Instruction*, Register*);
int bne(Instruction*, Register*);
int slt(Instruction*, Register*);


#endif //SIMPLECALCULATOR_INSTRUCTION_H
