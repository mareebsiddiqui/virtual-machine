#include "vm.hpp"

VirtualMachine::VirtualMachine(int argc, const char* argv[]){
    for (int j = 1; j < argc; ++j){
        if (!read_image(argv[j])){
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }
    /*setting up VM*/
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;
    running = 1;
}

void VirtualMachine::shutdown(){
    restore_input_buffering();
}

void VirtualMachine::read_image_file(FILE* file){
    /* the origin tells us where in memory to place the image */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    /* we know the maximum file size so we only need one fread */
    uint16_t max_read = UINT16_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    /* swap to little endian */
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}

int VirtualMachine::read_image(const char* image_path){
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0; };
    VirtualMachine::read_image_file(file);
    fclose(file);
    return 1;
}

uint16_t VirtualMachine::check_key(){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
};

/* Memory Access */
void VirtualMachine::mem_write(uint16_t address, uint16_t val){
    memory[address] = val;
}

uint16_t VirtualMachine::mem_read(uint16_t address) {
    if (address == MR_KBSR){
        if (check_key()){
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else{
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

void VirtualMachine::update_flags(uint16_t r){
    if (reg[r] == 0){
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15){ /* a 1 in the left-most bit indicates negative */
        reg[R_COND] = FL_NEG;
    }
    else{
        reg[R_COND] = FL_POS;
    }
};

void VirtualMachine::fetch_execute(){
    while(running){
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;  //opcode fetched
    
        switch (op) {
            case OP_ADD:
                /* ADD */
                {
                    /* destination register (DR) */
                    uint16_t r0 = (instr >> 9) & 0x7;
                    /* first operand (SR1) */
                    uint16_t r1 = (instr >> 6) & 0x7;
                    /* whether we are in immediate mode */
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
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t imm_flag = (instr >> 5) & 0x1;
                
                    if (imm_flag)
                    {
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                        reg[r0] = reg[r1] & imm5;
                    }
                    else
                    {
                        uint16_t r2 = instr & 0x7;
                        reg[r0] = reg[r1] & reg[r2];
                    }
                    update_flags(r0);
                }

                break;
            case OP_NOT:
                /* NOT */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                
                    reg[r0] = ~reg[r1];
                    update_flags(r0);
                }

                break;
            case OP_BR:
                /* BR */
                {
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    uint16_t cond_flag = (instr >> 9) & 0x7;
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
                    uint16_t r1 = (instr >> 6) & 0x7;
                    reg[R_PC] = reg[r1];
                }

                break;
            case OP_JSR:
                /* JSR */
                {
                    uint16_t long_flag = (instr >> 11) & 1;
                    reg[R_R7] = reg[R_PC];
                    if (long_flag)
                    {
                        uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
                        reg[R_PC] += long_pc_offset;  /* JSR */
                    }
                    else
                    {
                        uint16_t r1 = (instr >> 6) & 0x7;
                        reg[R_PC] = reg[r1]; /* JSRR */
                    }
                    break;
                }

                break;
            case OP_LD:
                /* LD */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    reg[r0] = mem_read(reg[R_PC] + pc_offset);
                    update_flags(r0);
                }

                break;
            case OP_LDI:
                /* LDI */
                {
                    /* destination register (DR) */
                    uint16_t r0 = (instr >> 9) & 0x7;
                    /* PCoffset 9*/
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    /* add pc_offset to the current PC, look at that memory location to get the final address */
                    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                    update_flags(r0);
                }

                break;
            case OP_LDR:
                /* LDR */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t offset = sign_extend(instr & 0x3F, 6);
                    reg[r0] = mem_read(reg[r1] + offset);
                    update_flags(r0);
                }

                break;
            case OP_LEA:
                /* LEA */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    reg[r0] = reg[R_PC] + pc_offset;
                    update_flags(r0);
                }

                break;
            case OP_ST:
                /* ST */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    mem_write(reg[R_PC] + pc_offset, reg[r0]);
                }

                break;
            case OP_STI:
                /* STI */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
                }

                break;
            case OP_STR:
                /* STR */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t offset = sign_extend(instr & 0x3F, 6);
                    mem_write(reg[r1] + offset, reg[r0]);
                }

                break;
            case OP_TRAP:
                /* TRAP */
                switch (instr & 0xFF)
                {
                    case TRAP_GETC:
                        /* TRAP GETC */
                        /* read a single ASCII char */
                        reg[R_R0] = (uint16_t)getchar();

                        break;
                    case TRAP_OUT:
                        /* TRAP OUT */
                        putc((char)reg[R_R0], stdout);
                        fflush(stdout);

                        break;
                    case TRAP_PUTS:
                        /* TRAP PUTS */
                        {
                            /* one char per word */
                            uint16_t* c = memory + reg[R_R0];
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
                        puts("HALT");
                        fflush(stdout);
                        running = 0;

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