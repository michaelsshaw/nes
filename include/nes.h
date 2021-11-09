#ifndef NES_NES_H_
#define NES_NES_H_

#include <cpu.h>

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
    * $4018-$401F	$0008	APU and I/O functionality that is normally disabled.
    * https://wiki.nesdev.org/w/index.php?title=CPU_memory_map
*/

#define OFFS_CART 0x4020

struct ppu;

struct nes
{
    struct cpu *cpu;
    struct ppu *ppu;

    struct cartridge
    {
        u8 mapper;
        u8 s_prg_rom_16;
        u8 s_chr_rom_8;

        u8 *prg;
        u8 *chr;
    } cartridge;

    void **mappers;

    int offs;
};

void
nes_irq(struct nes *nes);

void
nes_nmi(struct nes *nes);

void
nes_reset(struct nes *nes);

extern struct nes *NES;

#endif // NES_NES_H_