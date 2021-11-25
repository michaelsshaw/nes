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

#ifndef NES_NES_H_
#define NES_NES_H_

/*! @file nes.h */

#include <cpu.h>

/*!
 * @mainpage NES
 *
 * This is the documentation for a NES emulator written by Michael Shaw.
 *
 * The program begins in the #main function in nes.c
 *
 *
 * @see nes_cpu_read()
 * @see nes_cpu_write()
 */

/*

    **** CPU memory map
    * Address range	Size	Device
    * $0000-$07FF	$0800	2KB internal RAM
    * $ 0800-$0FFF	$0800	Mirrors of $0000-$07FF
    * $1000-$17FF	$0800
    * $1800-$1FFF	$0800
    * $2000-$2007	$0008	NES PPU registers
    * $2008-$3FFF	$1FF8	Mirrors of $2000-2007 (repeats every 8 bytes)
    * $4000-$4017	$0018	NES APU and I/O registers
    * $4018-$401F	$0008	APU and I/O functionality that is normally
   disabled.
    * https://wiki.nesdev.org/w/index.php?title=CPU_memory_map
*/

#define OFFS_CART  0x4020
#define NES_WIDTH  256
#define NES_HEIGHT 240
#define NES_RES    (NES_WIDTH * NES_HEIGHT)

struct ppu;

/*!
 * @struct nes
 * Data structure representation of the entire emulation
 */
struct nes
{
    struct cpu *cpu; //!< 6502 CPU data structure
    struct ppu *ppu; //!< PPU structure

    struct
    {
        u8 mapper;
        u8 s_prg_rom_16;
        u8 s_chr_rom_8;

        u8 *prg;
        u8 *chr;
    } cartridge; //!< Contains all ROM cartridge data

    u16 mirror;

    void **mappers; //!< Function array for memory mappers

    int offs;

    u8 enable;

    u32 pixels[NES_WIDTH * NES_HEIGHT];

    uint64_t cycle;
};

void
nes_irq(struct nes *nes);

void
nes_nmi(struct nes *nes);

void
nes_reset(struct nes *nes);

extern struct nes *NES;

#endif // NES_NES_H_
