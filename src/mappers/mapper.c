#include "../util.h"
#include "mapper.h"

#define MODE(_a) if (mode == _a)

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

    return (mem + addr);
}