/*
 * MIT License
 *
 * Copyright 2021 Michael Shaw
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef NES_PPU_H_
#define NES_PPU_H_

/*! @file ppu.h */

#include <cpu.h>
#include <nes.h>
#include <stdio.h>

#define PPUCTRL   0x2000
#define PPUMASK   0x2001
#define PPUSTATUS 0x2002
#define OAMADDR   0x2003
#define OAMDATA   0x2004
#define PPUSCROLL 0x2005
#define PPUADDR   0x2006
#define PPUDATA   0x2007
#define OAMDMA    0x4014 // ppu registers locations in cpu ram

#define PPUCTRL_V   0x80 //
#define PPUCTRL_P   0x40 //
#define PPUCTRL_H   0x20 //
#define PPUCTRL_B   0x10 //
#define PPUCTRL_S   0x08 //
#define PPUCTRL_I   0x04 //
#define PPUCTRL_NN  0x03 //
#define PPUMASK_BGR 0xE0 //
#define PPUMASK_s   0x10 //
#define PPUMASK_b   0x08 //
#define PPUMASK_M   0x04 //
#define PPUMASK_m   0x02 //
#define PPUMASK_G   0x01 //
#define PPUSTATUS_V 0x80 //
#define PPUSTATUS_S 0x40 //
#define PPUSTATUS_O 0x20 // Bitmasks for ppu flags

#define PPULOOPY_FY 0x7000
#define PPULOOPY_Y  0x0800
#define PPULOOPY_X  0x0400
#define PPULOOPY_CY 0x03E0
#define PPULOOPY_CX 0x001F

// Register made by loopy
// Layout: (0 FFFYXCC CCCVVVVV)
// Fine y scroll(F), nametable_y(Y), nametable_x(X),
// coarse_y(C), coarse_x(V)

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
        u8 ppuctrl;   //!< CPU address $2000
        u8 ppumask;   //!< CPU address $2001
        u8 ppustatus; //!< CPU address $2002
        u8 oamaddr;   //!< CPU address $2003
        u8 oamdata;   //!< CPU address $2004
        u8 ppuscroll; //!< CPU address $2005
        u8 ppuaddr;   //!< CPU address $2006
        u8 ppudata;   //!< CPU address $2007
    } registers;      //!< PPU internal 8-bit registers

    u8 cpu_latch;
    u8 nmi;
    u8 address_latch;
    u8 data; //!< Data return from the address in PPUDATA

    u16 vaddr;
    u16 taddr; //!< Address written by the CPU at PPUADDR

    u16 fxscroll; //!< Fine X scroll
    u8  fstoggle; //!< Fine scroll toggle

    u16 reg_shift_1;
    u16 reg_shift_2;

    int cycle;    //!< Cycle count
    int scanline; //!< Current scanline in rendering

    u32 pal[0x40]; //!< All 64 colors the NES can display

    struct nes *fw; //!< NES data structure that also contains this structure

    /*
     * 8-bit registers that store the info for the next tile
     */

    u8 bg_id;  //!< Background next tile location
    u8 bg_at;  //!< Background next tile attribute
    u8 bg_lsb; //!< Background next tile least-significant byte
    u8 bg_msb; //!< Background next tile most-significant byte

    /*
     * 16-bit shift registers for rendering the background include:
     */
    u16 bg_shift_plo; //!< Pattern table low
    u16 bg_shift_phi; //!< Pattern table high
    u16 bg_shift_alo; //!< Attribute table low
    u16 bg_shift_ahi; //!< Attribute table high

    /*
     * Sprite registers
     */

    u8 oam[256]; //!< Object attribute memory
    u8 soam[32]; //!< Secondary OAM
    u8 n_oam;    // 0-63
    u8 m_oam;    // 0-3
    u8 i_soam;   // 0-7
    u8 soam_true;
    u8 soam_write_disable;
    u8 soam_latch;

    u8 sprite_count;

    u8 sp_shift_lo[8];
    u8 sp_shift_hi[8];
    u8 sp_latch[8];
    u8 sp_counter[8];
};

void
ppu_debug_print_oam(struct ppu *ppu);

/*!
 * Nametable mirroring enum
 */
enum ppumirror
{
    MIRROR_HORIZONTAL = 0x2BFF,
    MIRROR_VERTICAL   = 0x27FF,
    MIRROR_OS_LO,
    MIRROR_OS_HI
};

/*!
 * Allows the CPU to write to one of the exposed registers in the cpu address
 * range $2000-$3FFF (mirrored every 8 bits to $2000-2007)
 *
 * @param ppu
 * @param addr Address of the PPU register to write to
 * @param data Data to write to the PPU
 *
 * @see #ppu_cpu_read
 */
void
ppu_cpu_write(struct ppu *ppu, u16 addr, u8 data);

/*!
 * Allows the CPU to read one of the exposed registers in the cpu address range
 * $2000-$3FFF (mirrored every 8 bits to $2000-2007)
 *
 * @param ppu
 * @param addr Address of the PPU register to write to
 *
 * @see #ppu_cpu_write
 */
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
 * # TLDR:
 *      Runs a single clock cycle for the PPU and computes a
 *      single pixel
 *
 *
 * ## Overview:
 *      The NES will render 261 scanlines, which I will refer to as S(-1,260).
 *      Every scanline is rendered across 341 cycles C(0,340).
 *
 *      The resolution of the NES is 256 x 240, so only scanlines S(0,239) are
 *      actually output to the screen, with the rest being run at the end of
 *      each rendering period, referred to as the "vertical blanking" period.
 *      This is the only time in which the CPU is safe to actually output data
 *      via PPUDATA or OAMDATA/OAMDMA as doing so otherwise would cause
 *      artifacting and other weird glitches.
 *
 *
 * ### Background rendering:
 *
 *      Shift registers are updated every 8 cycles from C(2-257)
 *      The most significant bit is grabbed from each of the 4 bg
 *      shift registers and used to form a pixel.
 *
 *      https://wiki.nesdev.org/w/index.php?title=PPU_scrolling
 *      https://wiki.nesdev.org/w/index.php?title=PPU_rendering
 *
 *      These articles contain very detailed information on how
 *      shifters work and the reasoning behind a lot of the bitwise
 *      manipulation that I performed a great deal of, without
 *      realizing that there was already an article containing every
 *      single number I computed myself.
 *
 *
 * @param ppu
 */
void
ppu_clock(struct ppu *ppu);

#endif // NES_PPU_H_
