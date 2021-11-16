#ifndef NES_PPU_H_
#define NES_PPU_H_

/*! @file ppu.h */

#include <cpu.h>
#include <nes.h>

#define PPUCTRL     0x2000
#define PPUMASK     0x2001
#define PPUSTATUS   0x2002
#define OAMADDR     0x2003
#define OAMDATA     0x2004
#define PPUSCROLL   0x2005
#define PPUADDR     0x2006
#define PPUDATA     0x2007
#define OAMDMA      0x4014 // ppu registers locations in cpu ram

#define PPUCTRL_V   0x80   //
#define PPUCTRL_P   0x40   //
#define PPUCTRL_H   0x20   //
#define PPUCTRL_B   0x10   //
#define PPUCTRL_S   0x08   //
#define PPUCTRL_I   0x04   //
#define PPUCTRL_NN  0x03   //
#define PPUMASK_BGR 0xE0   //
#define PPUMASK_s   0x10   //
#define PPUMASK_b   0x08   //
#define PPUMASK_M   0x04   //
#define PPUMASK_m   0x02   //
#define PPUMASK_G   0x01   //
#define PPUSTATUS_V 0x80   //
#define PPUSTATUS_S 0x40   //
#define PPUSTATUS_O 0x20   // Bitmasks for ppu flags

#define PPURMASK(_a) ((_a) == PPUSTATUS ? (0xE0) : 0xFF)

/*!
 * @struct ppu
 * Data structure representation of the PPU
 */
struct ppu
{
    u8 *vram; //!< Byte array containing the nametables

    u32 *(pattern_tables_pix[2]); //!< 2 debug pixel arrays for displaying
                                  //!< pattern tables

    struct
    {
        u8 ppuctrl;
        u8 ppumask;
        u8 ppustatus;
        u8 oamaddr;
        u8 oamdata;
        u8 ppuscroll;
        u8 ppuaddr;
        u8 ppudata;
    } registers; //!< PPU internal 8-bit registers

    u8 cpu_latch;
    u8 address_latch;
    u8 data; //!< Data return from the address in PPUDATA

    u16 vaddr;     //!< Address written by the CPU at PPUADDR
    u8 vaddrtemp; //!< Temporary storage of the upper byte of the address
                   //!< written at PPUADDR
    u16 fxscroll;  //!< Fine X scroll
    u8  fstoggle;  //!< Fine scroll toggle

    u16 reg_shift_1;
    u16 reg_shift_2;

    u16 cycle;    //!< Cycle count
    u16 scanline; //!< Current scanline in rendering

    u32 pal[0x40]; //!< All 64 colors the NES can display

    struct nes *fw; //!< NES data structure that also contains this structure
};

/*!
 * Notifies the PPU that one of its registers has been accessed by the CPU by
 * through the memory addresses $2000-$3FFF
 *
 * @param nes NES data structure containing the CPU and PPU
 * @param addr The address accessed by the CPU, masked between $2000-$2007
 * @param rdonly Whether or not this was a read or write access
 * @param data If this was a write, the data written to the register
 *
 * @returns 1 if the operation should be blocked, 0 if allowed
 */
u8
ppu_touch(struct nes *nes, u16 addr, u8 rdonly, u8 data);

void
ppu_cpu_write(struct ppu *ppu, u16 addr, u8 data);

u8
ppu_cpu_read(struct ppu *ppu, u16 addr);

void
ppu_init(struct ppu *ppu);

void
ppu_free(struct ppu *ppu);

/*!
 * **DEBUG FUNCTION**
 *
 * Builds a 32-bit integer pixel array representing a pattern table on the ROM
 *
 * @param ppu
 * @param i Pattern table to be read (0 or 1)
 * @param pal Palette to be used
 */
u32 *
ppu_get_patterntable(struct ppu *ppu, u8 i, u8 pal);

/*!
 * Reads from somewhere in the PPU's addressable range
 *
 * @param ppu
 * @param addr Address to be read
 *
 * @returns 8-bit value at memory location addr
 */
u8
ppu_read(struct ppu *ppu, u16 addr);

/*!
 * Writes to somewhere in the PPU's addressable range
 *
 * @param ppu
 * @param addr Address to be read
 * @param val Value to be written
 */
void
ppu_write(struct ppu *ppu, u16 addr, u8 val);

/*!
 * Writes to somewhere in the PPU's addressable range
 *
 * @param ppu
 */
void
ppu_clock(struct ppu *ppu);

#endif // NES_PPU_H_