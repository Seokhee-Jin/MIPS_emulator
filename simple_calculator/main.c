#include "instruction.h"
#include "string_to_instruction.h"
#include <stdio.h>
#include <unistd.h>

//#define FILENAME "../input4.txt"

int main() {
    // 작업 디렉토리 표시
    char buff[1024];
    getcwd( buff, 1024);
    printf( "Present working directory: %s\n", buff);

    // 텍스트 파일 경로 입력
    char FILENAME[50];
    printf("Enter file name: ");
    scanf("%s", FILENAME);

    // 구조체 변수 선언 및 초기화
    Register my_register = {
            .register_val_array = {0},
            .register_status_array = {false}
    };

    Instruction** instructions = file_to_instructions(FILENAME); // 이게 현재 문제임.

    int jump = 0;
    int end = count_lines(FILENAME);

    int i = 0;
    while(1){
        jump = run_instruction(instructions[i], &my_register);
        print_register(&my_register);
        i = (jump>0) ? jump-1 : i+1;
        if (i >= end) {
            break;
        }
    }

    printf("%s", "Press 'q' to exit.");
    while (getchar()!='q'){
    }

    return 0;
}