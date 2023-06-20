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
    u_int32_t readData1, readData2; //signed output
    u_int32_t writeData;
} register_file_t;

typedef struct control_s {
    bool ALUSrc;
    bool RegDest, RegWrite;
    bool MemRead, MemWrite, MemtoReg;
    bool Branch, Jump, BrTaken;
    bool ALUOp1, ALUOp0;
} control_t;

typedef struct stats_s {
    int type_R, type_I, type_J;
    int br_taken;
    int mem_access;
} stats_t;

// == pipeline registers (IF/ID, ID/EX, EX/MEM, MEM/WB) == //
// * pipelined control: 0-RegDst, 1-ALUOp1, 2-ALUOp0, 3-ALUSrc, / 4-Branch, 5-MemRead, 6-MemWrite, / 7-RegWrite, 8-MemtoReg
typedef struct if_id_s {
    u_int32_t pc;
    u_int32_t inst;
} if_id_t;

typedef struct id_ex_s {
    bool EX_M_WB[9]; // pipelined control
    u_int32_t pc; // should be deleted for final version
    u_int32_t readData1, readData2; // signed output
    u_int32_t ext_imm; // sign-extented immediate
    u_int32_t rt, rd; //
} id_ex_t;

typedef struct ex_mem_s {
    bool M_WB[5]; // pipelined control
    u_int32_t adder_result;
    // u_int32_t zero; // todo: ?? needed?
    u_int32_t alu_result;
    u_int32_t readData2;
    u_int32_t writeReg;

} ex_mem_t;

typedef struct mem_wb_s {
    bool WB[2]; // pipelined control
    u_int32_t readData;
    u_int32_t alu_result;
    u_int32_t writeReg;
} mem_wb_t;

// ========================= //
// == Function Prototypes == //
// ========================= //

// read all instructions from a binary
void read_instructions(char *filename);
// fetch a instruction from the inst_mem
void fetch_instruction();
// reset all control signals
void reset_control();
// determine Control unit's control values from opcode
void set_control(u_int32_t opcode, control_t *control, char *opname);
// decode an instruction accrording to instruction type todo: 수정하기
void decode_instruction();
// Get alu_control value determined by ALUOp control or funct field of decoded instruction todo: 수정하기
void set_alu_control(bool ALUOp1, bool ALUOp0, u_int32_t funct, u_int32_t *alu_control, char *opname);
// ALU's behavior depends on alu_control
u_int32_t alu(u_int32_t alu_control, u_int32_t input1, u_int32_t input2);
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

// == for pipeline == //
bool detect_hazard();

// ====================== //
// == Global Variables == //
// ====================== //

int cycle_num = 1;
//u_int32_t pc_next = 0;
//u_int32_t pc_now = 0;
u_int32_t pc_reg = 0;

u_int32_t alu_result_signal = 0;
u_int32_t read_data_signal = 0;

u_int32_t r[NUM_OF_REGISTERS] = {0, }; // 32개의 레지스터 값... 전부 0.
// u_int32_t ext_imm = 0;

u_int32_t inst_mem[0x1000000 / 4] = {0, };
u_int32_t data_mem[0x1000000 / 4] = {0, };

stats_t stats = {0, };

bool terminated = false;

// == pipeline global variables == //
if_id_t if_id = {0, };
id_ex_t id_ex = {0, };
ex_mem_t ex_mem = {0, };
mem_wb_t mem_wb = {0, };



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

void reset_control(control_t *control) {
    memset(control, 0, sizeof(control_t));
}


void fetch_instruction(){
    printf("\nCycle[%d] (PC: 0X%X)\n", cycle_num, pc_reg);
    cycle_num++;

    u_int32_t fetched_inst = inst_mem[pc_reg / 4];

    if (fetched_inst) {
        printf("  [Fetch Instruction] %08X\n", fetched_inst);
    } else {
        printf("  [Fetch Instruction] No operation!\n");
    }

    // Store values in the pipeline register.

    // When Load-Use Data Hazard detected, prevent update of IF/ID register.
    if (!detect_hazard()){
        if_id.pc = pc_reg + 4; // todo: more gracefully...
        if_id.inst = fetched_inst;
    }
}


void decode_instruction(){
    // Load values from the pipeline register.
    u_int32_t pc = if_id.pc;
    u_int32_t fetched_inst = if_id.inst;

    // < 1. decode a instruction >
    instruction_t inst = {0, }; // This variable was global in the previous single-cycle project.

    inst.opcode = fetched_inst / (int)pow(2, 26);
    inst.rs = fetched_inst % (int)pow(2, 26) / pow(2, 21) ;
    inst.rt = fetched_inst % (int)pow(2, 21) / pow(2, 16);
    inst.rd = fetched_inst % (int)pow(2, 16) / pow(2, 11);
    inst.shamt = fetched_inst % (int)pow(2, 11) / pow(2, 6);
    inst.funct = fetched_inst % (int)pow(2, 6);
    inst.imm = fetched_inst % (int)pow(2, 16);
    inst.addr = fetched_inst % (int)pow(2, 26);

    // Sign-Extension
    u_int32_t ext_imm;
    if (inst.imm >= pow(2, 15)){
        u_int32_t t = 0xffff0000;
        ext_imm = t + inst.imm; // ext_imm is a 32-bit sign extended value of inst.imm.
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


    // <2. determine "control_t control">  // pipelined control (4-4.19p)
    control_t control = {0, };

    char opname[20] = "";


    reset_control(&control);
    set_control(inst.opcode, &control, opname);

//    u_int32_t *alu_control = (u_int32_t *)malloc(sizeof(u_int32_t)); todo: alu_control is in the execute stage.
//    *alu_control = 0xFFFFFFFF;
//    set_alu_control(control.ALUOp1, control.ALUOp0, inst.funct, alu_control, opname);

    // <3. register read>
    u_int32_t readData1, readData2;
    readData1 = r[inst.rs];
    readData2 = r[inst.rt];

    //reg_file.writeReg = control.RegDest? inst.rd: inst.rt; // todo: writeReg would be determined in the execution stage.
    //reg_file.writeReg = control.Jump? 31: reg_file.writeReg; // for "jal" operation // todo: **important** this feature should be added again in any stage.



    // <4. Calculate address and determine next pc > // Control Hazard: early computation of branch (p.349)

    // <4-1. determine the PCSrc control signal>
    // zero test logic
    bool is_zero = ((readData1 - readData2) == 0);

    bool Jump = (inst.opcode == 2 || inst.opcode == 3);
    bool BrTaken = false;
    if (inst.opcode == 4){ // beq
        BrTaken = control.Branch && (is_zero == true); // branch
    } else if (inst.opcode == 5){ //bne
        BrTaken = control.Branch && (is_zero == false);
    }

    bool PCSrc1 = Jump;
    bool PCSrc2 = BrTaken;

    // <4-2. determine the next pc value>
    if (detect_hazard()){ // When Load-Use Data Hazard detected, prevent update of pc.
        printf("  [PC Update] PC = 0X%X (stall) \n", pc_reg);

    } else if (inst.funct == 0x8) { // when funct field is "0x8", jr operation occurs.
        pc_reg = r[inst.rs];

        printf("  [PC Update] PC <- 0X%X (jr) \n", pc_reg);

    } else {
        if (!PCSrc2 && !PCSrc1) { // PC + 4
            pc_reg = pc;
            printf("  [PC Update] PC <- 0X%X\n", pc_reg);

        } else if (PCSrc2 && !PCSrc1) { // Br Taken
            pc_reg = pc + (ext_imm * 4);
            printf("  [PC Update] PC <- 0X%X = 0X%X + 0X%X (branch)\n", pc_reg, pc, (ext_imm * 4));

        } else { // Jump
            pc_reg = inst.addr * 4 + (u_int32_t)(floor(pc / pow(2, 28)) * pow(2, 28)); // shift left 2 and concat.
            printf("  [PC Update] PC <- 0X%X (jump)\n", pc_reg);
        }
    }

    if (pc_reg == 0xFFFFFFFF) { // todo: still needed?
        terminated = true;
    }


    //todo: new forwarding unit needed // Data hazard for branches (4-5.24p, 350p)


    if (fetched_inst) {
        printf("  [Decode Instruction] Type: %c, Name: %s\n", inst.type, opname); // todo: opname can be also determined in the execution statge.
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

    // Store values in the pipeline register.
    if (detect_hazard()) {
        // When Load-Use Data Hazard detected, force control values in ID/EX register to 0. // todo: need to check nop is occurring
        id_ex.EX_M_WB[0] = id_ex.EX_M_WB[1] = id_ex.EX_M_WB[2] = id_ex.EX_M_WB[3] =
        id_ex.EX_M_WB[4] = id_ex.EX_M_WB[5] = id_ex.EX_M_WB[6] = id_ex.EX_M_WB[7] =
        id_ex.EX_M_WB[8] = false;
    } else {
        id_ex.EX_M_WB[0] = control.RegDest;
        id_ex.EX_M_WB[1] = control.ALUOp1;
        id_ex.EX_M_WB[2] = control.ALUOp0;
        id_ex.EX_M_WB[3] = control.ALUSrc;
        id_ex.EX_M_WB[4] = control.Branch;
        id_ex.EX_M_WB[5] = control.MemRead;
        id_ex.EX_M_WB[6] = control.MemWrite;
        id_ex.EX_M_WB[7] = control.RegWrite;
        id_ex.EX_M_WB[8] = control.MemtoReg;
    }
    id_ex.pc = pc;
    id_ex.readData1 = readData1;
    id_ex.readData2 = readData2;
    id_ex.ext_imm = ext_imm;
    id_ex.rt = inst.rt;
    id_ex.rd = inst.rd;
}

void set_control(u_int32_t opcode, control_t *control, char *opname) {
    // Programmable Logic Array Implementation

    switch (opcode) {
        case 0x0: // R_format
            control->RegDest = true;
            control->RegWrite = true;
            control->ALUOp1 = true;
            break;
        case 0x2:
            strcpy(opname, "j");
            control->Jump = true;
            break;
        case 0x3:
            strcpy(opname, "jal");
            control->Jump = true;
            control->RegWrite = true;
            break;
        case 0x4:
            strcpy(opname, "beq");
            control->Branch = true;
            control->ALUOp0 = true;
            break;
        case 0x5:
            strcpy(opname, "bne");
            control->Branch = true;
            control->ALUOp0 = true;
            break;
        case 0x8:
            strcpy(opname, "addi");
            control->ALUSrc = true;
            control->RegWrite = true;
            break;
        case 0x9:
            strcpy(opname, "addiu, li");
            control->ALUSrc = true;
            control->RegWrite = true;
            break;
        case 0xa:
            strcpy(opname, "slti");
            control->ALUSrc = true;
            control->RegWrite = true;
            break;
        case 0x23:
            strcpy(opname, "lw");
            control->ALUSrc = true;
            control->MemRead = true;
            control->MemtoReg = true;
            control->RegWrite = true;
            break;
        case 0x2b:
            strcpy(opname, "sw");
            control->ALUSrc = true;
            control->MemWrite = true;
            break;
        default:
            fprintf(stderr, "\nopcode error: 0X%X", opcode);
            exit(1);
    }
}

void set_alu_control(bool ALUOp1, bool ALUOp0, u_int32_t funct, u_int32_t *alu_control, char *opname) {

    if (ALUOp1  == 0){
        *alu_control = ALUOp0? 6 : 2; // sub : add
    } else {
        switch (funct) {
            case 0x0:
                opname = "nop";
                *alu_control = 0xFFFFFFFF;
                break;
            case 0x8:
                opname = "jr"; // PC = R[rs] + 0
                *alu_control = 2; // add
                break;
            case 0x20: // add
                opname = "add";
                *alu_control = 2; // add
                break;
            case 0x21: // addu
                opname = "addu";
                *alu_control = 2; // add
                break;
            case 0x22: // sub
                opname = "sub";
                *alu_control = 6; // sub
                break;
            case 0x24: // and
                opname = "and";
                *alu_control = 0; // and
                break;
            case 0x25: // or, move
                opname = "or, move";
                *alu_control = 1; // or
                break;
            case 0x2a: // slt
                opname = "slt";
                *alu_control = 7; // set less than
                break;

            default:
                fprintf(stderr, "\nfunct error: 0X%X", funct);
                exit(1);
        }

    }
}

u_int32_t alu(u_int32_t alu_control, u_int32_t input1, u_int32_t input2) {

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

// Load-Use Data hazard (4-5.17p, 343p).
bool detect_hazard() {
    u_int32_t fetched_inst = if_id.inst;
    u_int32_t if_id_rs = fetched_inst % (int)pow(2, 26) / pow(2, 21) ;
    u_int32_t if_id_rt = fetched_inst % (int)pow(2, 21) / pow(2, 16);

    return (id_ex.EX_M_WB[5] && ((id_ex.rt == if_id_rs) || (id_ex.rt == if_id_rt)));
}

void execute(){
    // Load values from the pipeline register.
    u_int32_t *control = id_ex.EX_M_WB;
    u_int32_t pc = id_ex.pc;
    u_int32_t readData1 = id_ex.readData1;
    u_int32_t readData2 = id_ex.readData2;
    u_int32_t ext_imm = id_ex.ext_imm;
    u_int32_t rt = id_ex.rt;
    u_int32_t rd = id_ex.rd;

    u_int32_t input1 = readData1;
    u_int32_t input2 = control[3] ? ext_imm : readData2; //ALUSrc

    // <1. Execute Operation>
    u_int32_t alu_result = alu(input1, input2);
    if (alu_result != 0xFFFFFFFF) {
        printf("  [Execute Operation] Operation: %s\n", op_name);
    }

    // <2. Calculate address>
    // determine the BrTaken control signal.
    bool BrTaken = 0;
    if (inst.opcode == 4){ // beq
        BrTaken = control[4] && (alu_result == 0); // branch
    } else if (inst.opcode == 5){ //bne
        BrTaken = control[4] && (alu_result != 0);
    }
    bool PCSrc1 = Jump;
    bool PCSrc2 = BrTaken;

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
            pc_next = inst.addr * 4 + (u_int32_t)((u_int32_t)(pc_now / pow(2, 28)) * pow(2, 28)); // shift left 2 and concat.
            printf("  [PC Update] PC <- 0X%X (jump)\n", pc_next);
        }
    }

    if (pc_next == 0xFFFFFFFF) {
        terminated = true;
    }

    // Store values in the pipeline register.

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