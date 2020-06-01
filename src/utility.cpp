#include "utility.hpp"

/* Sign Extend */
uint16_t sign_extend(uint16_t x, int bit_count){
    // let bit_count = i
    // if ith bit of x from the right is 1
    // set bits from left most to x equal to 1
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    
    // else just return as is
    return x;
}

/* Swap */
uint16_t swap16(uint16_t x){
    // swap the first eight bits with the last eight bits
    // as the instructions are in big endian by default
    return (x << 8) | (x >> 8);
}
