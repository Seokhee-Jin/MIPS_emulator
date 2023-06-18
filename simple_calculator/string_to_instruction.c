//
// Created by 진석희 on 2023/04/04.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "string_to_instruction.h"
#include "instruction.h"

int count_lines(const char* filename) {
    /** 공백 라인을 전부 무시하고 문자가 있는 라인만 세는 함수  **/
    int count = 0;
    char c;
    FILE* fp = fopen(filename, "r");
    int is_blank = 1;

    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n' && !is_blank) {
            count++;
            is_blank = 1;
        } else if (c != ' ' && c != '\t' && c != '\n') {
            is_blank = 0;
        }
    }

    if (!is_blank) {
        count++;
    }

    fclose(fp);
    return count;
}


Instruction** get_instructions(char** lines, int num_lines) {
    /** 문자열의 배열을 즉 명령어 문장들을 Instruction구조체의 배열로 변환.**/

    Instruction** ins_array = (Instruction**) malloc(sizeof(Instruction*) * num_lines);

    for (int i = 0; i < num_lines; i++) {
        char* opcode = strtok(lines[i], " ");
        char* operand1 = strtok(NULL, " ");
        char* operand2 = strtok(NULL, " ");
        char* operand3 = strtok(NULL, " ");

        Instruction* ins = (Instruction*) malloc(sizeof(Instruction));
        ins->opcode = opcode ? strdup(opcode) : NULL;
        ins->operand1 = operand1 ? strdup(operand1) : NULL;
        ins->operand2 = operand2 ? strdup(operand2) : NULL;
        ins->operand3 = operand3 ? strdup(operand3) : NULL;

        ins_array[i] = ins;
    }

    return ins_array;
}


Instruction** file_to_instructions(char* filename){
    /** 텍스트 파일을 Instruction 구조체의 배열로 반환하기 위한 함수. **/
    //=== 전문을 하나의 스트링으로====//
    FILE* file_ptr = fopen(filename, "r");

    if (file_ptr == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    char* full_str = (char*) malloc(MAX_FILE_LEN* sizeof(char));
    char c;
    int char_count = 0;

    // 한 글자씩 전부 읽어서 문자열에 저장.
    while((c = fgetc(file_ptr)) != EOF){
        full_str[char_count] = c;
        char_count++;
    }
    full_str[char_count] = '\0';
    fclose(file_ptr);

    //===전문 스트링을 문장 스트링 배열로===//
    char* lines[MAX_FILE_LEN/MAX_LINE_LEN];
    char* ptr;
    int line_count = 0;
    lines[line_count] = strtok(full_str, "\n");
    line_count++;
    while((ptr = strtok(NULL, "\n")) != NULL){ //
        lines[line_count] = ptr;
        line_count++;
    }

    //===최종 변환===//
    Instruction** instructions;
    instructions = get_instructions(lines, line_count);

    return instructions;
}

int str_to_constant(const char* str) {
    /** 16진수 문자열 일 경우 정수형 반환. 그렇지 않으면 exit code **/
    size_t len = strlen(str);

    // "0x"로 시작하지 않거나 3글자 미만 시 에러 발생.
    if (len < 3 || (str[0] != '0' || tolower(str[1]) != 'x')) {
        fprintf(stderr, "Invalid hexadecimal value: %s\n", str);
        exit(EXIT_FAILURE);
    }

    // 16진수 문자열을 숫자로 변환.
    int num = 0;
    for (size_t i = 2; i < len; i++) {
        // 세번째 문자부터 16진수 숫자(ex:4, A, F..)인지 확인 및 에러 처리.
        if (!isxdigit(str[i])) {
            fprintf(stderr, "Invalid hexadecimal value: %s\n", str);
            exit(EXIT_FAILURE);
        }
        // 점화식을 사용해 16진수 변환. '0'~'9'의 숫자 문자일 경우 문자 '0'을 빼서 정수형에 대응시킴.
        // 그 외 16진수 알파벳일 경우 소문자로 변환 후 'a'를 뺴고 10을 더해서 정수형에 대응시킴.
        num = num * 16 + (isdigit(str[i]) ? str[i] - '0' : tolower(str[i]) - 'a' + 10);
    }

    return num;
}


int str_to_register_index(const char* str){
    /** "r0", "r1",.. 형태의 레지스터를 나타내는 문자열이면 r을 제거해서 정수형 숫자(인덱스)를 반환. 예외 발생시 exit **/
    size_t len = strlen(str);

    // "r"로 시작하지 않거나 2글자 미만 시 에러 발생.
    if (len < 2 || tolower(str[0] != 'r')) {
        fprintf(stderr, "Invalid register_val_array value (r0, r1, ...): %s\n", str);
        exit(EXIT_FAILURE);
    }

    // "r숫자"에서 숫자만 추출.
    int num = 0;
    for (size_t i = 1; i < len; i++) {
        // 두번째 문자부터 10진수 숫자(0~9)인지 확인 및 예외 처리
        if (!isdigit(str[i])) {
            fprintf(stderr, "Invalid register_val_array value (r0, r1, ...): %s\n", str);
            exit(EXIT_FAILURE);
        }
        // 점화식을 사용해 10진수 변환. '0'~'9'의 숫자 문자에서 문자 '0'을 빼서 정수형에 대응시킴.
        num = num * 10 + str[i] - '0';
    }

    // 레지스터 인데스 범위 체크 및 예외 처리.
    if (num < 0 || num >= NUM_REGISTERS) {
        fprintf(stderr, "Out of register_val_array's index range (0~%d): %d\n", NUM_REGISTERS - 1, num);
        exit(EXIT_FAILURE);
    }

    return num;
}
