#include "instruction.h"
#include "string_to_instruction.h"
#include <stdio.h>
//#define FILENAME "../input4.txt"

int main() {
    char FILENAME[255];
    printf("Enter file name: ");
    scanf("%s", FILENAME);
    //
    Register my_register = {
            .register_val_array = {0},
            .register_status_array = {false}
    };
    Instruction** instructions = file_to_instructions(FILENAME);


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

    return 0;
}