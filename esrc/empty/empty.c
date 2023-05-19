#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpmbdos.h"
#include "cprintf.h"
#include "syslib/cpm_sysfunc.h"
#include "syslib/ansi_term.h"

#define BUF_SIZE 128
static uint8_t dma_buffer[BUF_SIZE];

void sys_init(void) {
	cpm_sysfunc_init();
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))

uint8_t hexbyte(uint8_t sum, uint8_t byte) {
    cprintf("%02x", byte);
    return sum - byte;
}

uint8_t hexword(uint8_t sum, uint16_t word) {
    sum = hexbyte(sum, word >> 8);
    return hexbyte(sum, word);
}

#define HEXSIZE 32
void print_buf(size_t offset) {
    size_t i, idx;
    for(idx=0; idx<BUF_SIZE; idx += HEXSIZE) {
        size_t count = MIN(HEXSIZE, BUF_SIZE-idx);
        uint8_t sum = 0;
        cprintf(":");
        sum = hexbyte(sum, count);
        sum = hexword(sum, idx+offset);
        sum = hexbyte(sum, 0); // record type 0
        for(i=0; i < count; i++) {
            sum = hexbyte(sum, dma_buffer[idx+i]);
        }
        hexbyte(sum, sum);
        cprintf("\n");
    }
}

uint8_t bios_conin(void) {
    BDOSCALL b = {B_CONIN};
    return cpmbios(&b);
}

uint8_t bios_const(void) {
    BDOSCALL b = {B_CONST};
    return cpmbios(&b);
}

void bios_conout(uint8_t c) {
    BDOSCALL b = {B_CONOUT, c<<8};
    cpmbios(&b);
}

int main() {
	FCB *fcb_ptr = NULL;
	uint8_t rval;
	sys_init();

        dprintf(bios_conout, "Printing via BIOS\n");

	fcb_ptr = malloc(sizeof(FCB));

	memset(fcb_ptr, 0, sizeof(FCB));
	cpm_setFCBname("empty", "com", fcb_ptr);
	rval = cpm_performFileOp(fop_open, fcb_ptr);
        if(rval != 0) { cprintf("error %02x\n", rval); return EXIT_FAILURE; }

        size_t offset = 0x100;
        while(1) {
            cpm_setDMAAddr((uint16_t)dma_buffer);
            rval = cpm_performFileOp(fop_readSeqRecord, fcb_ptr);
            if(rval != 0) { break; }
            print_buf(offset);
            offset += BUF_SIZE;
            break;
        }

        cprintf(":00000001FF\n");

        cprintf("Press any key");
        while(!bios_const()) {};
        cprintf("\nKey pressed, finding out what it is\n");
        cprintf("You pressed character #%d\n", (int)bios_conin());

	return (EXIT_SUCCESS);
}

