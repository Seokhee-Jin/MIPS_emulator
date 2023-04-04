//
// Created by 진석희 on 2023/04/03.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "instruction.h"
#include "string_to_instruction.h"

void print_register(Register* reg_p){
    int* vals = reg_p->register_val_array;
    bool* status = reg_p->register_status_array;

    for(int i = 0; i < NUM_REGISTERS; i++){
        if (status[i]){
            printf("r%d = %d, ", i, vals[i]);
        }
    }
    printf("%s", "\n");
}

void print_instruction(Instruction* instruction){
    printf("opcode: %s, operand1: %s, operand2: %s, operand3: %s\n",
           instruction->opcode,
           instruction->operand1,
           instruction->operand2,
           instruction->operand3);
}

int run_instruction(Instruction* ins_p, Register* reg_p){
    // 프린트는 메인 루프에서 하자. 리턴이랑 맞물리지가 않아서..
    if (strcmp("ADD", ins_p->opcode) == 0) {
        return add(ins_p, reg_p);
    } else if (strcmp("SUB", ins_p->opcode) == 0){
        return sub(ins_p, reg_p);
    } else if (strcmp("MUL", ins_p->opcode) == 0){
        return mul(ins_p, reg_p);
    } else if (strcmp("DIV", ins_p->opcode) == 0){
        return div_(ins_p, reg_p);
    } else if (strcmp("MOV", ins_p->opcode) == 0){
        return mov(ins_p, reg_p);
    } else if (strcmp("LW", ins_p->opcode) == 0){
        return lw(ins_p, reg_p);
    } else if (strcmp("SW", ins_p->opcode) == 0){
        return sw(ins_p, reg_p);
    } else if (strcmp("RST", ins_p->opcode) == 0){
        return rst(ins_p, reg_p);
    } else if (strcmp("JMP", ins_p->opcode) == 0){
        return jmp(ins_p, reg_p);
    } else if (strcmp("BEQ", ins_p->opcode) == 0){
        return beq(ins_p, reg_p);
    } else if (strcmp("BNE", ins_p->opcode) == 0){
        return bne(ins_p, reg_p);
    } else if (strcmp("SLT", ins_p->opcode) == 0){
        return slt(ins_p, reg_p);
    } else if (ins_p->opcode == NULL){
        exit(EXIT_SUCCESS);
    } else {
        fprintf(stderr, "Wrong operation: %s", ins_p->opcode);
        exit(EXIT_FAILURE);
    }
}

int add(Instruction* ins_p, Register* reg_p){
    // 피연산자 레지스터 인덱스
    int* reg_array = reg_p->register_val_array;
    int op1_reg_idx = str_to_register_index(ins_p->operand1);
    int op2_reg_idx = str_to_register_index(ins_p->operand2);
    int op3_reg_idx = str_to_register_index(ins_p->operand3);

    // 레지스터 활성화
    bool* reg_status_array = reg_p->register_status_array;
    reg_status_array[op1_reg_idx] = true;

    // 연산
    reg_array[op1_reg_idx] = reg_array[op2_reg_idx] + reg_array[op3_reg_idx];
    return 0;
}

int sub(Instruction* ins_p, Register* reg_p){
    // 피연산자 레지스터 인덱스
    int* reg_array = reg_p->register_val_array;
    int op1_reg_idx = str_to_register_index(ins_p->operand1);
    int op2_reg_idx = str_to_register_index(ins_p->operand2);
    int op3_reg_idx = str_to_register_index(ins_p->operand3);

    // 레지스터 활성화
    bool* reg_status_array = reg_p->register_status_array;
    reg_status_array[op1_reg_idx] = true;

    // 연산
    reg_array[op1_reg_idx] = reg_array[op2_reg_idx] - reg_array[op3_reg_idx];
    return 0;
}

int mul(Instruction* ins_p, Register* reg_p){
    // 피연산자 레지스터 인덱스
    int* reg_array = reg_p->register_val_array;
    int op1_reg_idx = str_to_register_index(ins_p->operand1);
    int op2_reg_idx = str_to_register_index(ins_p->operand2);
    int op3_reg_idx = str_to_register_index(ins_p->operand3);

    // 레지스터 활성화
    bool* reg_status_array = reg_p->register_status_array;
    reg_status_array[op1_reg_idx] = true;

    // 연산
    reg_array[op1_reg_idx] = reg_array[op2_reg_idx] * reg_array[op3_reg_idx];
    return 0;
}

int div_(Instruction* ins_p, Register* reg_p){
    // 피연산자 레지스터 인덱스
    int* reg_array = reg_p->register_val_array;
    int op1_reg_idx = str_to_register_index(ins_p->operand1);
    int op2_reg_idx = str_to_register_index(ins_p->operand2);
    int op3_reg_idx = str_to_register_index(ins_p->operand3);

    // 레지스터 활성화
    bool* reg_status_array = reg_p->register_status_array;
    reg_status_array[op1_reg_idx] = true;

    // 연산
    reg_array[op1_reg_idx] = reg_array[op2_reg_idx] / reg_array[op3_reg_idx];
    return 0;
}

int mov(Instruction* ins_p, Register* reg_p){ //TODO 작동?
    // 피연산자 레지스터 인덱스
    int* reg_array = reg_p->register_val_array;
    int op1_reg_idx = str_to_register_index(ins_p->operand1);

    // 레지스터인지 아닌지에 따른 분기문
    if ((ins_p->operand2)[0] == 'r'){
        reg_array[op1_reg_idx] =  reg_array[str_to_register_index(ins_p->operand2)];
    } else {
        reg_array[op1_reg_idx] = str_to_constant(ins_p->operand2);
    }

    // 레지스터 활성화
    bool* reg_status_array = reg_p->register_status_array;
    reg_status_array[op1_reg_idx] = true;

    return 0;
}

int lw(Instruction* ins_p, Register* reg_p){
    // 피연산자 레지스터 인덱스
    int* reg_array = reg_p->register_val_array;
    int op1_reg_idx = str_to_register_index(ins_p->operand1);

    // 레지스터 활성화
    bool* reg_status_array = reg_p->register_status_array;
    reg_status_array[op1_reg_idx] = true;

    // 연산
    reg_array[op1_reg_idx] = str_to_constant(ins_p->operand2);
    return 0;
}

int sw(Instruction* ins_p, Register* reg_p){
    // 피연산자 레지스터인지만 테스트
    str_to_register_index(ins_p->operand1);

    // 프린트 해서 결과만 출력할 것
    return 0;
}

int rst(Instruction* ins_p, Register* reg_p){
    // 모든 레지스터 비활성화
    for (int i = 0; i < NUM_REGISTERS; i++){
        (reg_p->register_status_array)[i] = false;
    }
    return 0;
}

int jmp(Instruction* ins_p, Register* reg_p){
    // 리턴값을 배열의 인덱스로 사용하여 명령어를 점프할 것임.
    return str_to_constant(ins_p->operand1);
}

int beq(Instruction* ins_p, Register* reg_p){
    int* reg_array = reg_p->register_val_array;
    int op1 = reg_array[str_to_register_index(ins_p->operand1)];

    // 레지스터인지 아닌지에 따른 분기문
    int op2 = ((ins_p->operand2)[0] == 'r')? reg_array[str_to_register_index(ins_p->operand2)] : str_to_constant(ins_p->operand2);

    // 1,2 피연산자가 같으면 점프
    if (op1 == op2){
        return str_to_constant(ins_p->operand3);
    } else {
        return 0;
    }
}

int bne(Instruction* ins_p, Register* reg_p){
    int* reg_array = reg_p->register_val_array;
    int op1 = reg_array[str_to_register_index(ins_p->operand1)];

    // 레지스터인지 아닌지에 따른 분기문
    int op2 = ((ins_p->operand2)[0] == 'r')? reg_array[str_to_register_index(ins_p->operand2)] : str_to_constant(ins_p->operand2);

    // 1,2 피연산자가 다르면 점프
    if (op1 != op2){
        return str_to_constant(ins_p->operand3);
    } else {
        return 0;
    }
}

int slt(Instruction* ins_p, Register* reg_p){
    int* reg_array = reg_p->register_val_array;
    int op1_reg_idx = str_to_register_index(ins_p->operand1);

    // 레지스터인지 아닌지에 따른 분기문
    int op2 = ((ins_p->operand2)[0] == 'r')? reg_array[str_to_register_index(ins_p->operand2)] : str_to_constant(ins_p->operand2);
    int op3 = ((ins_p->operand3)[0] == 'r')? reg_array[str_to_register_index(ins_p->operand3)] : str_to_constant(ins_p->operand3);

    // 피연산자2가 피연산자3보다 작으면 피연산자1을 1로 셋. 아니면 0으로 셋.
    if (op2 < op3){
        reg_array[op1_reg_idx] = 1;
    } else {
        reg_array[op1_reg_idx] = 0;
    }

    // 레지스터 활성화
    bool* reg_status_array = reg_p->register_status_array;
    reg_status_array[op1_reg_idx] = true;

    return 0;
}

