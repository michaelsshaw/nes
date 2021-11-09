#include "util.h"
#include "mapper.h"

#define MODE(_a) if (mode == _a)

void
mappers_init()
{
    MAP_DECL(00);
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
            //printf("\n%04X\n", addr);
            return nes->cartridge.chr + addr;
        }
    }

    return (mem + addr);
}