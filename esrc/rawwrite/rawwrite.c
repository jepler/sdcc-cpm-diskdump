#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpmbdos.h"
#include "cprintf.h"
#include "syslib/cpm_sysfunc.h"
#include "syslib/ansi_term.h"

#define SECTOR_SIZE 128

putchar_func_t printer;
#define reader sio_b_in

void sys_init(void) {
    cpm_sysfunc_init();
}

const uint8_t xlate[] = {1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9, 15, 21, 2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22};

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

uint8_t bios_write(void) {
    BDOSCALL b = {B_WRITE};
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

void sio_b_out(uint8_t c) {
    while(!(X810_SIO_B_CONTROL & 4)) { /* NOTHING */ }
    X810_SIO_B_DATA = c;
}

uint8_t sio_b_available(void) {
    return (X810_SIO_B_CONTROL & 1);
}

uint8_t sio_b_in(void) {
    while(!(X810_SIO_B_CONTROL & 1)) { /* NOTHING */ }
    return X810_SIO_B_DATA;
}

int main() {
        uint16_t error_count=0;
	sys_init();

        X810_SIO_B_RATE = 0xf; // 19200 (*16) = 307200
        X810_SIO_B_CONTROL = 0x30; // reset channel
        sio_b_write_reg(4, 0x44); // 16x clock, 1 stop bit, no parity
        //sio_b_write_reg(4, 0x04); // 1x clock, 1 stop bit, no parity
        sio_b_write_reg(5, 0b01101010); // TX 8 bits, TX-enable, RTS-enable
        sio_b_write_reg(3, 0b11000001); // RX 8 bits, RX-enable

#if 0
        while(1) {
            if(bios_const()) {
                uint8_t c= bios_conin();
                if(c == 3) { return EXIT_SUCCESS;}
                sio_b_out(c);
            }
            if(sio_b_available()) {
                uint8_t c= sio_b_in();
                if(c == '\n') { bios_conout('\r'); bios_conout(c); }
                else if(c >32 && c <128) { bios_conout(c); }
                else { dprintf(bios_conout, "\\03o", c ); }
            }
        }
#endif
        dprintf(bios_conout, "paranoid? remove A: before hitting enter");
        while (!bios_const()) { }
        (void) bios_conin();

        printer = sio_b_out;
        // reader = sio_b_in;
        dprintf(bios_conout, "Writing B: from data via SIO B (printer) at 19200,8N1\n");
        dprintf(bios_conout, "(any key interrupts)\n");
        bios_seldsk(1);
        bios_home();
        for(int track=0; track<NUM_TRACKS; track++) {
            dprintf(bios_conout, "%2d ", track);
            printer('T');
            for(size_t i=0; i<sizeof(track_buffer); i++) {
                track_buffer[i] = reader();
            }
            bios_settrack(track);
            for(int logical_sector=0; logical_sector<NUM_SECTORS; logical_sector++) {
                int sector = xlate[logical_sector];
                if(bios_const()) { bios_conin(); goto interrupted; }
                bios_setsector(sector);
                bios_setdma(track_buffer + SECTOR_SIZE * (sector-1));
                bios_write();
                bios_conout('@' | sector);
            }
            bios_conout('\r');
            bios_conout('\n');
        }

        dprintf(bios_conout, "done\n", error_count);
	return (EXIT_SUCCESS);
interrupted:
        dprintf(bios_conout, "interrupted\n");
        return (EXIT_FAILURE);
}
