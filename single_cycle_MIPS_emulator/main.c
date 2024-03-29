#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#define MAX_FILENAME 100
#define NUM_OF_REGISTERS 32

// ================ //
// == Structures == //
// ================ //

typedef struct instruction_s {
    u_int32_t opcode, rs ,rt ,rd, shamt, funct, imm, addr;
    char type; // R, I, J
} instruction_t;

typedef struct register_file_s {
    u_int32_t readReg1, readReg2, writeReg; //input
    u_int32_t readData1, readData2;
    u_int32_t writeData;
} register_file_t;

typedef struct control_s {
    bool ALUSrc;
    bool RegDst, RegWrite;
    bool MemRead, MemWrite, MemtoReg;
    bool Branch, Jump, BrTaken;
    bool ALUOp1, ALUOp0;
} control_t;

typedef struct stats_s {
    int type_R, type_I, type_J;
    int br_taken;
    int mem_access;
} stats_t;

// ========================= //
// == Function Prototypes == //
// ========================= //

// read all instructions from a binary
void read_instructions(char *filename);
// fetch a instruction from the inst_mem

void fetch_instruction();
// reset all control signals
void reset_control();
// determine Control unit's control values from opcode and output operation name.
void opcode_to_control();
// decode an instruction accrording to instruction type
void decode_instruction();


// Get alu_control value determined by ALUOp control or funct field of decoded instruction
u_int32_t set_alu_control();
// ALU's behavior depends on alu_control
u_int32_t alu(u_int32_t input1, u_int32_t input2);
// execute operation or calculate address
void execute();

//read data from Data memory or store data to Data memory.
void access_memory();

//write the result back to a register
void write_back();

//update  statistics
void update_stats();

//print out the final result
void print_result();


// ====================== //
// == Global Variables == //
// ====================== //

int cycle_num = 1;
u_int32_t pc_next = 0;
u_int32_t pc_now = 0;
u_int32_t fetched_inst = 0;
u_int32_t alu_result_signal = 0;
u_int32_t read_data_signal = 0;

u_int32_t r[NUM_OF_REGISTERS] = {0, }; // 32개의 레지스터 값... 전부 0.
u_int32_t ext_imm = 0;

u_int32_t inst_mem[0x1000000 / 4] = {0, };
u_int32_t data_mem[0x1000000 / 4] = {0, };

control_t control = {0, };
instruction_t inst = {0, };
register_file_t reg_file = {0, };

char *op_name = "";
stats_t stats = {0, };

bool terminated = false;


// =================== //
// == Main function == //
// =================== //

int main(int argc, char *argv[]) {
    // 0. load binary file to inst_mem
    char* filename = argv[1];
    read_instructions(filename);

    while(!terminated){
        // 1. IF
        fetch_instruction();

        // 2. ID
        decode_instruction();

        // 3. EX
        execute();

        // 4. MEM
        access_memory();

        // 5. WB
        write_back();

        //update statistics
        update_stats();
    }

    print_result();

    return 0;
}

void read_instructions(char *filename){
    FILE *fp;
    fp = fopen(filename, "rb");

    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    // unsigned integer,  1 byte size
    u_int8_t buf[4];
    int bytes_read;
    int num_inst = 0;

    while ((bytes_read = fread(buf, 1, 4, fp)) != 0) {
        u_int32_t inst = 0;
        // for loops to create a whole instruction as big as 4bytes
        for (int i = 0; i < bytes_read; i++) {
            inst += buf[i] * pow(16, 6 - 2*i);
        }
        // instructions are loaded to memory one by one.
        inst_mem[num_inst] = inst;
        num_inst++;
    }

    fclose(fp);

    r[29] = 0x1000000; // stack pointer register
    r[31] = 0xFFFFFFFF; // return address register

    printf("=== read_instructions ===\n");
    printf("filename: %s\n", filename);
    printf("file size: %d bytes\n\n", num_inst * 4);

}

void reset_control() {
    memset(&control, 0, sizeof(control_t));
}


void fetch_instruction(){
    pc_now = pc_next;

    printf("\nCycle[%d] (PC: 0X%X)\n", cycle_num, pc_now);
    cycle_num++;

    fetched_inst = inst_mem[pc_now / 4];

    if (fetched_inst) {
        printf("  [Fetch Instruction] %08X\n", fetched_inst);
    } else {
        printf("  [Fetch Instruction] No operation!\n");
    }
}


void decode_instruction(){
    // decode a instruction.
    inst.opcode = fetched_inst / (int)pow(2, 26);
    inst.rs = fetched_inst % (int)pow(2, 26) / pow(2, 21) ;
    inst.rt = fetched_inst % (int)pow(2, 21) / pow(2, 16);
    inst.rd = fetched_inst % (int)pow(2, 16) / pow(2, 11);
    inst.shamt = fetched_inst % (int)pow(2, 11) / pow(2, 6);
    inst.funct = fetched_inst % (int)pow(2, 6);
    inst.imm = fetched_inst % (int)pow(2, 16);
    inst.addr = fetched_inst % (int)pow(2, 26);

    // ext_imm is a 32-bit sign extended value of inst.imm.
    if (inst.imm >= pow(2, 15)){
        u_int32_t t = 0xffff0000;
        ext_imm = t + inst.imm;
    } else {
        ext_imm = inst.imm;
    }

    if (inst.opcode == 2 || inst.opcode == 3){
        inst.type = 'J';
    } else if(inst.opcode == 0){
        inst.type = 'R';
    } else {
        inst.type = 'I';
    }


    // determine "control_t control"
    reset_control();
    opcode_to_control();

    // register read
    reg_file.readReg1 = inst.rs;
    reg_file.readReg2 = inst.rt;
    reg_file.writeReg = control.RegDst ? inst.rd : inst.rt;
    reg_file.writeReg = control.Jump? 31: reg_file.writeReg; // for "jal" operation
    reg_file.readData1 = r[reg_file.readReg1];
    reg_file.readData2 = r[reg_file.readReg2];

    if (fetched_inst) {
        printf("  [Decode Instruction] Type: %c\n", inst.type);
        if (inst.type == 'R') {
            printf("    opcode: 0X%X, rs: 0X%X (r[%d]=0X%X), rt: 0X%X (r[%d]=0X%X), rd: 0X%X (r[%d]=0X%X), shamt: 0X%X, funct: 0X%X\n",
                   inst.opcode, inst.rs, inst.rs, r[inst.rs], inst.rt, inst.rt, r[inst.rt], inst.rd, inst.rd, r[inst.rd],
                   inst.shamt, inst.funct);
        } else if (inst.type == 'I') {
            printf("    opcode: 0X%X, rs: 0X%X (r[%d]=0X%X), rt: 0X%X (r[%d]=0X%X), imm: 0X%X\n",
                   inst.opcode, inst.rs, inst.rs, r[inst.rs], inst.rt, inst.rt, r[inst.rt], inst.imm);
        } else if (inst.type == 'J') {
            printf("    opcode: 0X%X, addr: 0X%X\n", inst.opcode, inst.addr);
        }
    }
}

void opcode_to_control(){
    // Programmable Logic Array Implementation

    switch (inst.opcode) {
        case 0x0: // R_format
            control.RegDst = true;
            control.RegWrite = true;
            control.ALUOp1 = true;
            break;
        case 0x2: // j
            op_name = "j";
            control.Jump = true;
            break;
        case 0x3: // jal
            op_name = "jal";
            control.Jump = true;
            control.RegWrite = true;
            break;
        case 0x4: // beq
            op_name = "beq";
            control.Branch = true;
            control.ALUOp0 = true;
            break;
        case 0x5: // bne
            op_name = "bne";
            control.Branch = true;
            control.ALUOp0 = true;
            break;
        case 0x8:
            op_name = "addi";
            control.ALUSrc = true;
            control.RegWrite = true;
            break;
        case 0x9:
            op_name = "addiu, li";
            control.ALUSrc = true;
            control.RegWrite = true;
            break;
        case 0xa:
            op_name = "slti";
            control.ALUSrc = true;
            control.RegWrite = true;
            break;
        case 0x23:
            op_name = "lw";
            control.ALUSrc = true;
            control.MemRead = true;
            control.MemtoReg = true;
            control.RegWrite = true;
            break;
        case 0x2b:
            op_name = "sw";
            control.ALUSrc = true;
            control.MemWrite = true;
            break;
        default:
            fprintf(stderr, "\nopcode error: 0X%X", inst.opcode);
            exit(1);
    }
}

u_int32_t set_alu_control(){
    u_int32_t alu_control;
    if (control.ALUOp1  == 0){
        alu_control = control.ALUOp0 ? 6 : 2; // sub : add
    } else {
        switch (inst.funct) {
            case 0x0:
                op_name = "nop";
                alu_control = 0xFFFFFFFF;
                break;
            case 0x8:
                op_name = "jr"; // PC = R[rs] + 0
                alu_control = 2; // add
                break;
            case 0x20: // add
                op_name = "add";
                alu_control = 2; // add
                break;
            case 0x21: // addu
                op_name = "addu";
                alu_control = 2; // add
                break;
            case 0x22: // sub
                op_name = "sub";
                alu_control = 6; // sub
                break;
            case 0x24: // and
                op_name = "and";
                alu_control = 0; // and
                break;
            case 0x25: // or, move
                op_name = "or, move";
                alu_control = 1; // or
                break;
            case 0x2a: // slt
                op_name = "slt";
                alu_control = 7; // set less than
                break;

            default:
                fprintf(stderr, "\nfunct error: 0X%X", inst.funct);
                exit(1);
        }

    }
    return alu_control;
}

u_int32_t alu(u_int32_t input1, u_int32_t input2){
    u_int32_t alu_control = set_alu_control();
    u_int32_t alu_result;
    switch (alu_control) {
        case 0xFFFFFFFF:
            alu_result = 0xFFFFFFFF;
            break;
        case 0:
            alu_result = input1 * input2; // and ( &&보다 더 범용적일 듯 해서 *로 구현)
            break;
        case 1:
            alu_result = input1 + input2; // or ( || 보다 +가 나음. move가 컴파일 될 때의 funct는 0x25(or)로 나오기 때문..
                                        // detailed desc: move는 r[rd] = r[rs] + r[0] 라서 funct=0x20(add)로 컴파일 돼야할 것
                                        // 같지만 funct=0x25(or)로 컴파일 되기에 or은 덧셈 기능도 병행 할 수 있도록 구현해야 했음.
            break;
        case 2:
            alu_result = input1 + input2;
            break;
        case 6:
            alu_result = input1 - input2;
            break;
        case 7:
            alu_result = (input1 < input2)? 1: 0; //set less than
            break;
        case 12:
            alu_result = !(input1 || input2); // NOR function
            break;
        default:
            fprintf(stderr, "\nalu_control error: 0d%d", alu_control);
            exit(1);
    }

    return alu_result;
}

void execute(){
    u_int32_t input1 = reg_file.readData1;
    u_int32_t input2 = control.ALUSrc ? ext_imm : reg_file.readData2;

    // <1. Execute Operation>
    alu_result_signal = alu(input1, input2);
    if (alu_result_signal != 0xFFFFFFFF) {
        printf("  [Execute Operation] Operation: %s\n", op_name);
    }

    // <2. Calculate address>
    // determine the BrTaken control signal.
    control.BrTaken = 0;
    if (inst.opcode == 4){ // beq
        control.BrTaken = control.Branch && (alu_result_signal == 0);
    } else if (inst.opcode == 5){ //bne
        control.BrTaken = control.Branch && (alu_result_signal != 0);
    }
    bool PCSrc1 = control.Jump;
    bool PCSrc2 = control.BrTaken;

    // when funct field is "0x8", jr operation occurs.
    if (inst.funct == 0x8){
        pc_next = r[inst.rs];
        printf("  [PC Update] PC <- 0X%X (jr) \n", pc_next);

    } else {
        if (!PCSrc2 && !PCSrc1) { // PC + 4
            pc_next = pc_now + 4;
            printf("  [PC Update] PC <- 0X%X + 4\n", pc_now);

        } else if (PCSrc2 && !PCSrc1) { // Br Taken
            pc_next = (pc_now + 4) + (ext_imm * 4);
            printf("  [PC Update] PC <- (0X%X + 4) + 0X%X (branch)\n", pc_now, (ext_imm * 4));

        } else { // Jump
            pc_next = inst.addr * 4 + (u_int32_t)(floor(pc_now / pow(2, 28)) * pow(2, 28)); // shift left 2 and concat.
            printf("  [PC Update] PC <- 0X%X (jump)\n", pc_next);
        }
    }

    if (pc_next == 0xFFFFFFFF) {
        terminated = true;
    }
}


void access_memory(){
    u_int32_t address = alu_result_signal;
    u_int32_t write_data = reg_file.readData2;
    u_int32_t read_data = 0;

    if (control.MemWrite) {
        data_mem[alu_result_signal/4] = reg_file.readData2;
        printf("  [Access Memory] Store: data_mem[%d] <- 0X%X\n", alu_result_signal/4, reg_file.readData2);
    }

    if (control.MemRead) {
        read_data_signal = data_mem[alu_result_signal/4];
        printf("  [Access Memory] Load: data_mem[%d] = 0X%X\n", alu_result_signal/4, read_data_signal);
    }
}

void write_back(){
    reg_file.writeData = control.MemtoReg? read_data_signal: alu_result_signal;
    if (inst.opcode == 0x3) reg_file.writeData = (pc_now + 4); // for jal operation
    if (inst.opcode == 0xa) reg_file.writeData = (r[inst.rs] < ext_imm) ? 1 : 0; // for slti operation

    if (!fetched_inst) return; // if nop, then return.

    if (control.RegWrite) {
        if (reg_file.writeReg == 0) { // todo: is r[0] meaning the PC register?
            printf("  [Write Back] r[%d] = 0X%X (nothing changed)\n", reg_file.writeReg, r[reg_file.writeReg]);
        } else {
            r[reg_file.writeReg] = reg_file.writeData;
            printf("  [Write Back] r[%d] <- 0X%X\n", reg_file.writeReg, reg_file.writeData);
        }
    }
}


//update  statistics
void update_stats() {
    if (inst.type == 'R') stats.type_R++;
    if (inst.type == 'I') stats.type_I++;
    if (inst.type == 'J') stats.type_J++;
    if (control.MemRead || control.MemWrite) stats.mem_access++;
    if (control.BrTaken) stats.br_taken++;
}

//print out the final result
void print_result(){
    printf("\n ======== statistics =========\n");
    printf("\n Numbers of ... \n");
    printf("  type_R: %d\n", stats.type_R);
    printf("  type_I: %d\n", stats.type_I);
    printf("  type_J: %d\n", stats.type_J);
    printf("  br_taken: %d\n", stats.br_taken);
    printf("  mem_access: %d\n", stats.mem_access);
}