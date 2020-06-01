#ifndef VM
#define VM

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

#include <map>

#include "buffer.hpp" //buffering methods
#include "register.hpp" //enum of regs and traps
#include "utility.hpp" //read image and image files

class VirtualMachine{
  public:
    VirtualMachine(const char*);
    void start();
    void shutdown();

  private:
    /* Memory Storage */
    /* 2^16 locations */
    uint16_t memory[UINT16_MAX];

    /* Register Storage */
    uint16_t reg[R_COUNT];  
    bool running;

    int read_image(const char*);

    /* Memory Access */
    uint16_t check_key();
    void mem_write(uint16_t, uint16_t);
    uint16_t mem_read(uint16_t);

    void update_flags(uint16_t);

    void execute_instruction(uint16_t, uint16_t);
};

#endif