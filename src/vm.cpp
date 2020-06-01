#include "vm.hpp"

VirtualMachine::VirtualMachine(const char* object_file_path){
    if (!read_image(object_file_path)){
        printf("failed to load image: %s\n", object_file_path);
        exit(1);
    }
    
    // set handler for ctrl + c
    signal(SIGINT, handle_interrupt);

    // terminal input disabled
    disable_input_buffering();

    // set PC = 0x3000
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    running = true;
}

void VirtualMachine::shutdown(){
    restore_input_buffering();
}

int VirtualMachine::read_image(const char* image_path){
    FILE* file = fopen(image_path, "rb");
    
    if (!file) { return 0; };
    
    uint16_t origin;
    
    // first byte of the object file is the address
    // of origin of the program instructions in memory
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    // address space is 2^16 locations hence max program
    // size is (2^16 - origin) locations
    uint16_t max_read = UINT16_MAX - origin;
    
    // set pointer to first instruction
    uint16_t* p = memory + origin;
    
    // store all instruction in memory array and store
    // number of instructions
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    // convert each instruction to little endian
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }

    fclose(file);
    return 1;
}

uint16_t VirtualMachine::check_key(){
    // because the input buffering is disabled,
    // a file descriptor for stdin is required
    // to read a character from terminal input(stdin)

    // set stdin file descriptor and clear all others
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    // do not wait if stdin has no input and return
    // from the function
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
};

/* Memory Access */
void VirtualMachine::mem_write(uint16_t address, uint16_t val){
    // write val at address
    memory[address] = val;
}

uint16_t VirtualMachine::mem_read(uint16_t address) {
    // if the address being read is the address of
    // keyboard status register
    if (address == MR_KBSR){
        if (check_key()){ // check if some input has been received at stdin
            memory[MR_KBSR] = (1 << 15); // set status register to 1
            memory[MR_KBDR] = getchar(); // data register to the character in stdin
        }
        else{
            memory[MR_KBSR] = 0; // else set status register to 0
        }
    }
    return memory[address];
}

void VirtualMachine::update_flags(uint16_t r){
    // r is the result register

    if (reg[r] == 0){
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15){ // a 1 in the left-most bit indicates negative
        reg[R_COND] = FL_NEG;
    }
    else{
        reg[R_COND] = FL_POS;
    }
};

void VirtualMachine::start(){
    while(running){
        // read instruction at address in PC and
        // increment PC
        uint16_t instr = mem_read(reg[R_PC]++);
        
        // 4 bits from the left is the opcode
        uint16_t op = instr >> 12;

        switch (op) {
            case OP_ADD:
                /* ADD */
                {
                    /* destination register (DR) */
                    // first 3 bits after opcode
                    uint16_t r0 = (instr >> 9) & 0x7;

                    /* first operand (SR1) */
                    // 3 bits after DR
                    uint16_t r1 = (instr >> 6) & 0x7;

                    /* whether we are in immediate mode */
                    // the bit after SR1
                    uint16_t imm_flag = (instr >> 5) & 0x1;
                
                    if (imm_flag)
                    {
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                        reg[r0] = reg[r1] + imm5;
                    }
                    else
                    {
                        uint16_t r2 = instr & 0x7;
                        reg[r0] = reg[r1] + reg[r2];
                    }
                
                    update_flags(r0);
                }

                break;
            case OP_AND:
                /* AND */
                {
                    /* destination register (DR) */
                    uint16_t r0 = (instr >> 9) & 0x7;

                    /* first operand (SR1) */
                    uint16_t r1 = (instr >> 6) & 0x7;

                    /* whether we are in immediate mode */
                    uint16_t imm_flag = (instr >> 5) & 0x1;
                
                    if (imm_flag)
                    {
                        // extend the sign of the imm value(rightmost 5 bits)
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                        reg[r0] = reg[r1] & imm5;
                    }
                    else
                    {
                        // last 3 bits of instr
                        uint16_t r2 = instr & 0x7;
                        reg[r0] = reg[r1] & reg[r2];
                    }
                    update_flags(r0);
                }

                break;
            case OP_NOT:
                /* NOT */
                {
                    /* DR */
                    uint16_t r0 = (instr >> 9) & 0x7;
                    /* SR */
                    uint16_t r1 = (instr >> 6) & 0x7;
                
                    reg[r0] = ~reg[r1];
                    update_flags(r0);
                }

                break;
            case OP_BR:
                /* BR */
                {
                    /* offset value */
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    /* cond_flag = 1 << 0 || 1 << 1 || 1 << 2 */
                    uint16_t cond_flag = (instr >> 9) & 0x7;

                    // if cond_flag = status register
                    if (cond_flag & reg[R_COND])
                    {
                        reg[R_PC] += pc_offset;
                    }
                }

                break;
            case OP_JMP:
                /* JMP */
                {
                    /* Also handles RET */
                    /* r1 = register where the address of the jump is stored */
                    uint16_t r1 = (instr >> 6) & 0x7;

                    /* indirect addressing */
                    // jump PC to the address stored in reg[r1]
                    reg[R_PC] = reg[r1];
                }

                break;
            case OP_JSR:
                /* JSR */
                {
                    // is it a long imm val
                    uint16_t long_flag = (instr >> 11) & 1;

                    // retain current PC in R7
                    reg[R_R7] = reg[R_PC];

                    if (long_flag)
                    {
                        // if its a long imm val, add offset directly to PC
                        uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
                        reg[R_PC] += long_pc_offset;  /* JSR */
                    }
                    else
                    {
                        /* indirect addressing */
                        // if its not an imm val,
                        // its a register number where the jump address
                        // is stored
                        uint16_t r1 = (instr >> 6) & 0x7;
                        reg[R_PC] = reg[r1]; /* JSRR */
                    }
                    break;
                }

                break;
            case OP_LD:
                /* LD */
                {
                    /* DR */
                    uint16_t r0 = (instr >> 9) & 0x7;

                    /* 9 bit long offset from current instruction */
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    /* add offset to current PC and read the value at that location */
                    reg[r0] = mem_read(reg[R_PC] + pc_offset);
                    
                    update_flags(r0);
                }

                break;
            case OP_LDI:
                /* LDI */
                {
                    /* destination register (DR) */
                    uint16_t r0 = (instr >> 9) & 0x7;
                    
                    /* offset of 9 bits */
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    
                    // add offset to current PC and read memory at that address 
                    // to get another address, then read the value at that address
                    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                    
                    update_flags(r0);
                }

                break;
            case OP_LDR:
                /* LDR */
                {
                    /* DR */
                    uint16_t r0 = (instr >> 9) & 0x7;

                    /* BR (Base Register) */
                    uint16_t r1 = (instr >> 6) & 0x7;

                    /* Offset */
                    uint16_t offset = sign_extend(instr & 0x3F, 6);
                    
                    // add offset to the address in base register and read
                    // memory
                    reg[r0] = mem_read(reg[r1] + offset);
                    
                    update_flags(r0);
                }

                break;
            case OP_LEA:
                /* LEA */
                {
                    /* DR */
                    uint16_t r0 = (instr >> 9) & 0x7;
                    
                    /* offset */
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    /* add offset to current instruction and store that in r0 */
                    reg[r0] = reg[R_PC] + pc_offset;
                    update_flags(r0);
                }

                break;
            case OP_ST:
                /* ST */
                {
                    /* DR */
                    uint16_t r0 = (instr >> 9) & 0x7;

                    /* offset */
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    // write value stored in DR at location
                    // "current instruction" + offset
                    mem_write(reg[R_PC] + pc_offset, reg[r0]);
                }

                break;
            case OP_STI:
                /* STI */
                {
                    /* DR */
                    uint16_t r0 = (instr >> 9) & 0x7;

                    /* offset */
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    
                    // write value stored at the address in DR at location
                    // "current instruction"+offset
                    mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
                }

                break;
            case OP_STR:
                /* STR */
                {
                    /* DR */
                    uint16_t r0 = (instr >> 9) & 0x7;
                    
                    /* BR (Base Register) */
                    uint16_t r1 = (instr >> 6) & 0x7;

                    /* offset */
                    uint16_t offset = sign_extend(instr & 0x3F, 6);

                    // write value stored in DR at location 
                    // "address in base register" + offset
                    mem_write(reg[r1] + offset, reg[r0]);
                }

                break;
            case OP_TRAP:
                /* TRAP */
                // handle different types of interrupts
                switch (instr & 0xFF)
                {
                    case TRAP_GETC:
                        /* TRAP GETC */
                        /* read a single ASCII char */
                        reg[R_R0] = (uint16_t)getchar();

                        break;
                    case TRAP_OUT:
                        /* TRAP OUT */
                        /* output the char in R0 */
                        putc((char)reg[R_R0], stdout);
                        fflush(stdout);

                        break;
                    case TRAP_PUTS:
                        /* TRAP PUTS */
                        {
                            /* one char per word */
                            
                            // add relative address of word to base address
                            // of program to point at the start of the word
                            uint16_t* c = memory + reg[R_R0];

                            // word should end in null pointer in the end
                            while (*c)
                            {
                                putc((char)*c, stdout);
                                ++c;
                            }
                            fflush(stdout);
                        }

                        break;
                    case TRAP_IN:
                        /* TRAP IN */
                        /* read a char and output it to the screen */
                        {
                            printf("Enter a character: ");
                            char c = getchar();
                            putc(c, stdout);
                            reg[R_R0] = (uint16_t)c;
                        }

                        break;
                    case TRAP_PUTSP:
                        /* TRAP PUTSP */
                        {
                            /* one char per byte (two bytes per word)
                                here we need to swap back to
                                big endian format */
                            uint16_t* c = memory + reg[R_R0];
                            while (*c)
                            {
                                // print the 2nd word first and the
                                // 1st word second
                                char char1 = (*c) & 0xFF;
                                putc(char1, stdout);
                                char char2 = (*c) >> 8;
                                if (char2) putc(char2, stdout);
                                ++c;
                            }
                            fflush(stdout);
                        }

                        break;
                    case TRAP_HALT:
                        /* TRAP HALT */
                        /* stop the program */
                        puts("HALT");
                        fflush(stdout);
                        running = false;

                        break;
                }

                break;
            case OP_RES:
            case OP_RTI:
            default:
                /* BAD OPCODE */
                abort();

                break;
        }
    }
}