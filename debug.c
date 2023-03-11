#include <nes.h>
#include "debug.h"

int
nes_debug_loop(void *in)
{
    return 0;
}

void
debug_print_oam(struct nes *nes)
{
    struct ppu *ppu = nes->ppu;

    for (int i = 0; i < 8; i++)
    {
    }
}
