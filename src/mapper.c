#include <stdlib.h>

#include "util.h"
#include "mapper.h"

#define MODE(_a) if (mode == _a)

void
mappers_init(struct nes *nes)
{
    MAP_DECL(00);
    MAP_DECL(01);
}

void
mapper_init(struct nes *nes)
{
    switch (nes->cartridge.mapper)
    {
        case 1:
            break;
    }
}

MAP_FUNC(00)
{
    MODE(MAP_MODE_CPU)
    {
        IFINRANGE(addr, 0x8000, 0xFFFF)
        {
            return nes->cartridge.prg +
                   (addr & (nes->cartridge.s_prg_rom_16 > 1 ? 0x7FFF : 0x3FFF));
        }
    }

    MODE(MAP_MODE_PPU)
    {
        IFINRANGE(addr, 0x0000, 0x1FFF) //
        {
            // printf("\n%04X\n", addr);
            return nes->cartridge.chr + addr;
        }
    }

    return (mem + addr);
}

MAP_FUNC(01)
{
    MODE(MAP_MODE_CPU)
    {
        IFINRANGE(addr, 0x6000, 0x7FFF)
        {
            return nes->cartridge.prg_ram + (addr - 0x6000);
        }

        IFINRANGE(addr, 0x8000, 0xBFFF) {}
    }

    MODE(MAP_MODE_PPU) {}

    return (mem + addr);
}

