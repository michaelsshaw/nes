#include "../util.h"
#include "mapper.h"

MAP_FUNC(00)
{
    IFINRANGE(addr, 0x8000, 0xFFFF)
    {
        return 0x4020 +
               (addr & (nes->cartridge.s_prg_rom_16 > 1 ? 0x7FFF : 0x3FFF));
    }
}