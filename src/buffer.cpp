#include "buffer.hpp"

struct termios original_tio;

void disable_input_buffering(){
    // store current terminal input parameters in
    // terminal interface structure termios
    tcgetattr(STDIN_FILENO, &original_tio);

    // disable canonical mode for terminal and disable
    // echoing of characters on screen
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;

    // set the new parameters for terminal input immediately
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering(){
    // restore the original terminal input parameters
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal){
    restore_input_buffering();
    printf("\n");
    exit(-2);
}