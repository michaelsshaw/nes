#ifndef NES_PPU_H_
#define NES_PPU_H_

#include <cpu.h>

#define PPUCTRL   0x2000
#define PPUMASK   0x2001
#define PPUSTATUS 0x2002
#define OAMADDR   0x2003
#define OAMDATA   0x2004
#define PPUSCROLL 0x2005
#define PPUADDR   0x2006
#define PPUDATA   0x2007
#define OAMDMA    0x4014 // ppu registers locations in cpu ram

#define PPUCTRL_V  0x80 //
#define PPUCTRL_P  0x40 //
#define PPUCTRL_H  0x20 //
#define PPUCTRL_B  0x10 //
#define PPUCTRL_S  0x08 //
#define PPUCTRL_I  0x04 //
#define PPUCTRL_NN 0x03 //

#define PPUMASK_BGR 0xE0 //
#define PPUMASK_s   0x10 //
#define PPUMASK_b   0x08 //
#define PPUMASK_M   0x04 //
#define PPUMASK_m   0x02 //
#define PPUMASK_G   0x01 //

#define PPUSTATUS_V 0x80 //
#define PPUSTATUS_S 0x40 //
#define PPUSTATUS_O 0x20 // Bitmasks for ppu flags

struct ppu
{
    u8 *mem;

    struct registers
    {
        u8 ppuctrl;
        u8 ppumask;
        u8 ppustatus;
        u8 oamaddr;
        u8 oamdata;
        u8 ppuscroll;
        u8 ppuaddr;
        u8 ppudata;
    } registers;

    u16 vaddr;
    u16 vaddrtemp;
    u16 fxscroll;
    u8  fstoggle;

    u16 reg_shift_1;
    u16 reg_shift_2;

    u16 cycleno;
};

void
ppu_touch(struct nes *em, u16 addr);

#endif // NES_PPU_H_