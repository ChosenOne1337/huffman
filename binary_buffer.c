#include <limits.h>
#include <string.h>
#include "binary_buffer.h"

#define BUF_SIZE 16384

unsigned char inbuf[BUF_SIZE] = {0};
unsigned char outbuf[BUF_SIZE] = {0};

int INBUF_BYTE_POS = 0;
int OUTBUF_BYTE_POS = 0;
unsigned char INBUF_BIT_MASK = 1u;
unsigned char OUTBUF_BIT_MASK = 1u;

//read from/write to file

unsigned read_from_file(FILE *fInput) {
    inbuf_reset();
    return fread(inbuf, sizeof(char), BUF_SIZE, fInput);
}

unsigned write_to_file(FILE *fOutput) {
    int byte_num = OUTBUF_BYTE_POS + ((OUTBUF_BIT_MASK == 1u) ? 0 : 1);
    unsigned bytes_written = fwrite(outbuf, sizeof(char), byte_num, fOutput);
    outbuf_reset();
    return bytes_written;
}

//reset buffers

void outbuf_reset(void) {
    OUTBUF_BYTE_POS = 0;
    OUTBUF_BIT_MASK = 1u;
    memset(outbuf, 0, BUF_SIZE);
}

void inbuf_reset(void) {
    INBUF_BYTE_POS = 0;
    INBUF_BIT_MASK = 1u;
    memset(inbuf, 0, BUF_SIZE);
}

//check for buffer's end

inline int end_of_inbuf(void) {
    return INBUF_BYTE_POS == BUF_SIZE;
}

inline int end_of_outbuf(void) {
    return OUTBUF_BYTE_POS == BUF_SIZE;
}

//move to the next byte

int inbuf_next_byte(void) {
    //move to the next byte and update byte/bit index of the buffer
    ++INBUF_BYTE_POS;
    INBUF_BIT_MASK = 1u;
    return end_of_inbuf();
}

int outbuf_next_byte(void) {
    //move to the next byte and update byte/bit index of the buffer
    ++OUTBUF_BYTE_POS;
    OUTBUF_BIT_MASK = 1u;
    return end_of_outbuf();
}

//move to the next bit

inline int inbuf_next_bit(void) {
    return (INBUF_BIT_MASK <<= 1u) ? 0 : inbuf_next_byte();
}

inline int outbuf_next_bit(void) {
    return (OUTBUF_BIT_MASK <<= 1u) ? 0 : outbuf_next_byte();
}

//set or get a bit/byte at the current position

inline unsigned inbuf_get_bit(void) {
    return inbuf[INBUF_BYTE_POS] & INBUF_BIT_MASK;
}

inline unsigned inbuf_get_byte(void) {
    return inbuf[INBUF_BYTE_POS];
}

inline void outbuf_set_bit(void) {
    outbuf[OUTBUF_BYTE_POS] |= OUTBUF_BIT_MASK;
}

inline void outbuf_reset_bit(void) {
    outbuf[OUTBUF_BYTE_POS] &= ~OUTBUF_BIT_MASK;
}

inline void outbuf_set_byte(unsigned char byte) {
    outbuf[OUTBUF_BYTE_POS] = byte;
}
