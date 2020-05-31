#ifndef VM
#define VM

#include "buffer.hpp" //buffering methods
#include "register.hpp" //enum of regs and traps
#include "utility.hpp" //read image and image files

class VirtualMachine{
    /* Memory Storage */
    /* 65536 locations */
    uint16_t memory[UINT16_MAX];

    /* Register Storage */
    uint16_t reg[R_COUNT];  
    int running;  

    /* methods */
    public:
      VirtualMachine(int, const char*[]);
      void shutdown();
      void fetch_execute();

    private:
      void read_image_file(FILE*);
      int read_image(const char*);
      uint16_t check_key();

      /* Memory Access */
      void mem_write(uint16_t, uint16_t);
      uint16_t mem_read(uint16_t);

      void update_flags(uint16_t);
};

#endif