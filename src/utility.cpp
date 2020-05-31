#include "utility.hpp"

/* Sign Extend */
uint16_t sign_extend(uint16_t x, int bit_count){
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

/* Swap */
uint16_t swap16(uint16_t x){
    return (x << 8) | (x >> 8);
}
