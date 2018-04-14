#ifndef BINARY_BUFFER_H
#define BINARY_BUFFER_H

#include <stdio.h>

//read from/write to file

unsigned read_from_file(FILE *fInput);
unsigned write_to_file(FILE *fOutput);

//reset buffers

void inbuf_reset(void);
void outbuf_reset(void);

//check for buffer's end

int end_of_inbuf(void);
int end_of_outbuf(void);

//move to the next byte

int inbuf_next_byte(void);
int outbuf_next_byte(void);

//move to the next bit

int inbuf_next_bit(void);
int outbuf_next_bit(void);

//set or get a bit/byte at the current position

unsigned inbuf_get_bit(void);
unsigned inbuf_get_byte(void);

void outbuf_set_bit(void);
void outbuf_reset_bit(void);
void outbuf_set_byte(unsigned char byte);

#endif // BINARY_BUFFER_H
