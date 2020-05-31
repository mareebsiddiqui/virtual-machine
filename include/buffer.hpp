#ifndef BUFFER
#define BUFFER 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/termios.h>

extern void disable_input_buffering();
extern void restore_input_buffering();
extern void handle_interrupt(int);

#endif