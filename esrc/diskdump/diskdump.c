#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpmbdos.h"
#include "cprintf.h"
#include "syslib/cpm_sysfunc.h"
#include "syslib/ansi_term.h"

#define SECTOR_SIZE 128

putchar_func_t printer;

void sys_init(void) {
	cpm_sysfunc_init();
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))

const uint8_t xlate[] = {1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9, 15, 21, 2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22};

const uint8_t hexchars[] = "0123456789ABCDEF";
uint8_t hexbyte(uint8_t sum, uint8_t byte) {
    printer(hexchars[byte >> 4]);
    printer(hexchars[byte & 0xf]);
    return sum - byte;
}

uint8_t hexword(uint8_t sum, uint16_t word) {
    sum = hexbyte(sum, word >> 8);
    return hexbyte(sum, word);
}

#define IHEX_DATA (0)
#define IHEX_XERROR (0x81)
#define IHEX_XBLOCKADDR (0x80)
#define IHEX_XBLANK (0xE5)

void ihex_record(uint8_t count, uint16_t address, uint8_t rectype, uint8_t *buffer) {
    uint8_t sum = 0, i;
    printer(':');
    sum = hexbyte(sum, count);
    sum = hexword(sum, address);
    sum = hexbyte(sum, rectype);
    for(i=0; i < count; i++) {
        sum = hexbyte(sum, buffer[i]);
    }
    hexbyte(sum, sum);
    printer('\r');
    printer('\n');
}

void print_buf(size_t offset, uint8_t *buffer, size_t buf_size) {
    ihex_record(buf_size, offset, IHEX_DATA, buffer);
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
    BDOSCALL b = {B_CONOUT, c};
    cpmbios(&b);
}

void bios_seldsk(uint8_t c) {
    BDOSCALL b = {B_SELDSK, c};
    cpmbios(&b);
}

void bios_home(void c) {
    BDOSCALL b = {B_HOME};
    cpmbios(&b);
}


void bios_settrack(uint8_t c) {
    BDOSCALL b = {B_SETTRK, c | (c << 8)};
    cpmbios(&b);
}

void bios_setsector(uint8_t c) {
    BDOSCALL b = {B_SETSEC, c | (c << 8)};
    cpmbios(&b);
}

uint8_t bios_read(void) {
    BDOSCALL b = {B_READ};
    return cpmbios(&b);
}

uint8_t bios_setdma(void *ptr) {
    BDOSCALL b = {B_SETDMA, (uint16_t)ptr};
    return cpmbios(&b);
}

uint8_t all_e5(uint8_t *data_ptr) {
    uint8_t i;
    for(i=0; i<SECTOR_SIZE; i++) {
        if(data_ptr[i] != 0xe5) { return 0; }
    }
    return 1;
}

__sfr __at(0) X810_SIO_A_RATE;
__sfr __at(4) X810_SIO_A_DATA;
__sfr __at(6) X810_SIO_A_CONTROL;

__sfr __at(0xc) X810_SIO_B_RATE;
__sfr __at(5) X810_SIO_B_DATA;
__sfr __at(7) X810_SIO_B_CONTROL;

#define sio_b_write_reg(a, b) (X810_SIO_B_CONTROL = (a), X810_SIO_B_CONTROL = (b))


#define NUM_TRACKS (77)
#define NUM_SECTORS (26)
uint8_t track_buffer[SECTOR_SIZE * NUM_SECTORS];
uint8_t address[2];
uint8_t validity[NUM_SECTORS+1];

void sio_b_out(uint8_t c) {
    while(!(X810_SIO_B_CONTROL & 4)) { /* NOTHING */ }
    X810_SIO_B_DATA = c;
}

int main() {
	uint8_t rval;
        uint16_t error_count=0;
	sys_init();

        X810_SIO_B_RATE = 0xf; // 19200 (*16) = 307200
        // sio_b_write_reg(4, 0x44); // 16x clock, 1 stop bit, no parity
        sio_b_write_reg(4, 0x04); // 1x clock, 1 stop bit, no parity
        sio_b_write_reg(5, 0b01101010); // TX 8 bits, TX-enable, RTS-enable
        sio_b_write_reg(3, 0b11000001); // RX 8 bits, RX-enable

        printer = bios_conout;
        dprintf(bios_conout, "Patching\n");
        uint8_t *p_bios = *(uint8_t**)1 - B_WBOOT;
        dprintf(bios_conout, "bios @ %x\n", (uint16_t)p_bios);
        uint8_t *p_bios_read = *(uint8_t**)(p_bios+B_READ+1);
        dprintf(bios_conout, "read @ %x\n", (uint16_t)p_bios_read);
        print_buf((uint16_t)p_bios_read, p_bios_read, 32);
        if(p_bios_read[13] == 0xc8){
            p_bios_read[13] = 0xc9;
            printf("patched\n");
            print_buf((uint16_t)p_bios_read, p_bios_read, 32);
        }

        printer = sio_b_out;
        dprintf(bios_conout, "Dumping disk from B: via SIO B (printer) at 19200,8N1\n");
        dprintf(bios_conout, "(any key interrupts)\n");
        bios_seldsk(1);
        bios_home();
        for(int track=0; track<NUM_TRACKS; track++) {
            address[0] = track;
            dprintf(bios_conout, "%2d ", track);
            bios_settrack(track);
            for(int logical_sector=0; logical_sector<NUM_SECTORS; logical_sector++) {
                int sector = xlate[logical_sector];
                if(bios_const()) { bios_conin(); goto interrupted; }
                bios_setsector(sector);
                bios_setdma(track_buffer + SECTOR_SIZE * (sector-1));
                rval = bios_read();
                validity[sector] = rval == 0;
                if(rval != 0) {
                    bios_conout('!'); error_count++;
                } else {
                    bios_conout('@' | sector);
                }
            }
            dprintf(bios_conout, "\r%2d ", track);
            for(int sector=1; sector<=NUM_SECTORS; sector++) {
                if(bios_const()) { bios_conin(); goto interrupted; }
                address[1] = sector;
                uint8_t *data_ptr = track_buffer + SECTOR_SIZE * (sector-1);
                if(!validity[sector]) {
                    bios_conout('!');
                    ihex_record(2, 0, IHEX_XERROR, address);
                } else if(all_e5(data_ptr)) {
                    bios_conout('`' | sector);
                    ihex_record(2, 0, IHEX_XBLANK, address);
                } else {
                    bios_conout('@' | sector);
                    ihex_record(2, 0, IHEX_XBLOCKADDR, address);
                    print_buf(0, data_ptr, SECTOR_SIZE);
                }
            } 
            bios_conout('\r');
            bios_conout('\n');
        }

        dprintf(printer, ":00000001FF\n\x1a");
        dprintf(bios_conout, "done - %d errors\n", error_count);
	return (EXIT_SUCCESS);
interrupted:
        dprintf(bios_conout, "interrupted\n");
        return (EXIT_FAILURE);
}

