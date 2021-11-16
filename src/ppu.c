#include <stdlib.h>

#include <cpu.h>
#include <em6502.h>

#include "ppu.h"
#include "mapper.h"
#include "util.h"

#define PAL                           ppu->pal
#define FOR(_name, _a, _b)            for (u16(_name) = (_a); (_name) < (_b); (_name)++)
#define PTSETPIXEL(_pt, _x, _y, _val) (_pt)[(_x) + (_y)*128] = (_val)
#define PCOLREAD(_pal, _pix)                                                   \
    PAL[ppu_read(ppu, 0x3F00 + ((_pal) << 0x02) + _pix) & 0x3F]
// #define PCOLREAD(_pal, _pix)                                                   \
//    PAL[_pix & 0x3F + 8]

#define RNE(_cond)                                                             \
    if (!(_cond))                                                              \
    return 1

#define PPUREG(_ppu, _reg) *((u8 *)(&_ppu->registers + ((_reg)&0x0007)))
#define CLPPUFLAG(_ppu, _reg, _mask)                                           \
    PPUREG(_ppu, (_reg)) = PPUREG((_ppu), (_reg)) ^ (_mask)

// TODO: REDO THIS SHIT
// THIS WAS A TERRIBLE IDEA

u8
ppu_cpu_read(struct ppu *ppu, u16 addr)
{
    addr = addr & 0x0007;
    u8 r = 0x00;
    switch (addr)
    {
        case 0x0000:
            break;
        case 0x0001:
            break;
        case 0x0002:
            r = (ppu->registers.ppustatus | 0xE0) | (ppu->data & 0x1F);
            CLPPUFLAG(ppu, PPUSTATUS, PPUSTATUS_V);
            ppu->address_latch = 0;
            break;
        case 0x0003:
            break;
        case 0x0004:
            break;
        case 0x0005:
            break;
        case 0x0006:
            break;
        case 0x0007:
            r         = ppu->data;
            ppu->data = ppu_read(ppu, ppu->vaddr);
            if (ppu->vaddr >= 0x3F00)
                r = ppu->data;

            ppu->vaddr += (PPUREG(ppu, PPUCTRL) & PPUCTRL_I) ? 32 : 1;
            break;
    }

    return r;
}
#include <stdio.h>

void
ppu_cpu_write(struct ppu *ppu, u16 addr, u8 data)
{
    addr = addr & 0x0007;
    u8 r = 0x00;
    switch (addr)
    {
        case 0x0000:
            ppu->registers.ppuctrl = data;
            // some more stuff later
            break;
        case 0x0001:
            ppu->registers.ppumask = data;
            break;
        case 0x0002:
        case 0x0003:
        case 0x0004:
            break;
        case 0x0005:
            if (ppu->cpu_latch == 0)
            {
                ppu->fxscroll      = data & 0x07;
                ppu->address_latch = 1;
            }
            else
            {
                ppu->address_latch = 0;
            }
            break;
        case 0x0006:
            if (ppu->address_latch == 0)
            {
                ppu->vaddrtemp     = data;
                ppu->address_latch = 1;
            }
            else
            {
                ppu->vaddr         = (ppu->vaddrtemp << 0x08) | data;
                ppu->address_latch = 0;
            }
            break;
        case 0x0007:
            ppu_write(ppu, ppu->vaddr, data);
            ppu->vaddr += (PPUREG(ppu, PPUCTRL) & PPUCTRL_I) ? 32 : 1;
            break;
    }
}

u8 *
ppu_get_mempointer(struct ppu *ppu, u16 addr)
{
    u8 *mem = ppu->vram;

    struct nes *em = ppu->fw;
    IFINRANGE(addr, 0x3F00, 0x3FFF) //
    {
        u16 taddr = (addr & 0x001F);
        if (taddr == 0x0010)
            taddr = 0x0000;
        if (taddr == 0x0014)
            taddr = 0x0004;
        if (taddr == 0x0018)
            taddr = 0x0008;
        if (taddr == 0x001C)
            taddr = 0x000C;

        addr = 0x3F00 + taddr;
        return (mem + addr);

        // printf("%04X ", addr);
    }

    return MAP_CALL(em, em->cartridge.mapper, addr, mem, MAP_MODE_PPU);
}

u8
ppu_read(struct ppu *ppu, u16 addr)
{
    u8 *r = ppu_get_mempointer(ppu, addr);
    return *r;
}

void
ppu_write(struct ppu *ppu, u16 addr, u8 val)
{
    u8 *m = ppu_get_mempointer(ppu, addr);
    *m    = val;
}

u32 *
ppu_get_patterntable(struct ppu *ppu, u8 i, u8 pal)
{
    // ************************
    // THIS IS A DEBUG FUNCTION
    // ************************
    //
    // Purpose: Output a pattern table to a 128x128 u32 pixel array
    //
    // each pattern table contains 256 tiles in a 16x16 arrangement
    // each tile is 8x8 pixels
    // loop through 256 tiles

    FOR(ty, 0, 16)
    {
        FOR(tx, 0, 16)
        {
            u16 off = ty * 0x0100 + tx * 0x0010;
            FOR(row, 0, 8)
            {
                u8 lsb = ppu_read(ppu, i * 0x1000 + off + row);
                u8 msb = ppu_read(ppu, i * 0x1000 + off + row + 0x08);
                FOR(col, 0, 8)
                {
                    u8 pix = (lsb & 0x01) | ((msb & 0x01) << 0x01); //
                    lsb >>= 0x01; // we're reading from right to left here
                    msb >>= 0x01; // because of the & 0x01 operations above
                                  // printf(" pix: %d ", pix);
                    PTSETPIXEL(ppu->pattern_tables_pix[i],
                               tx * 8 + (7 - col),
                               ty * 8 + row,
                               PCOLREAD(pal, pix));
                }
            }
        }
    }

    return ppu->pattern_tables_pix[i];
}

void
ppu_clock(struct ppu *ppu)
{
}

void
pal_init(struct ppu *ppu)
{
    PAL[0x00] = 0xFF545454;
    PAL[0x01] = 0xFF001E74;
    PAL[0x02] = 0xFF081090;
    PAL[0x03] = 0xFF300088;
    PAL[0x04] = 0xFF440064;
    PAL[0x05] = 0xFF5C0030;
    PAL[0x06] = 0xFF540400;
    PAL[0x07] = 0xFF3C1800;
    PAL[0x08] = 0xFF202A00;
    PAL[0x09] = 0xFF083A00;
    PAL[0x0A] = 0xFF004000;
    PAL[0x0B] = 0xFF003C00;
    PAL[0x0C] = 0xFF00323C;
    PAL[0x0D] = 0xFF000000;
    PAL[0x0E] = 0xFF000000;
    PAL[0x0F] = 0xFF000000;
    PAL[0x10] = 0xFF989698;
    PAL[0x11] = 0xFF084CC4;
    PAL[0x12] = 0xFF3032EC;
    PAL[0x13] = 0xFF5C1EE4;
    PAL[0x14] = 0xFF8814B0;
    PAL[0x15] = 0xFFA01464;
    PAL[0x16] = 0xFF982220;
    PAL[0x17] = 0xFF783C00;
    PAL[0x18] = 0xFF545A00;
    PAL[0x19] = 0xFF287200;
    PAL[0x1A] = 0xFF087C00;
    PAL[0x1B] = 0xFF007628;
    PAL[0x1C] = 0xFF006678;
    PAL[0x1D] = 0xFF000000;
    PAL[0x1E] = 0xFF000000;
    PAL[0x1F] = 0xFF000000;
    PAL[0x20] = 0xFFECEEEC;
    PAL[0x21] = 0xFF4C9AEC;
    PAL[0x22] = 0xFF787CEC;
    PAL[0x23] = 0xFFB062EC;
    PAL[0x24] = 0xFFE454EC;
    PAL[0x25] = 0xFFEC58B4;
    PAL[0x26] = 0xFFEC6A64;
    PAL[0x27] = 0xFFD48820;
    PAL[0x28] = 0xFFA0AA00;
    PAL[0x29] = 0xFF74C400;
    PAL[0x2A] = 0xFF4CD020;
    PAL[0x2B] = 0xFF38CC6C;
    PAL[0x2C] = 0xFF38B4CC;
    PAL[0x2D] = 0xFF3C3C3C;
    PAL[0x2E] = 0xFF000000;
    PAL[0x2F] = 0xFF000000;
    PAL[0x30] = 0xFFECEEEC;
    PAL[0x31] = 0xFFA8CCEC;
    PAL[0x32] = 0xFFBCBCEC;
    PAL[0x33] = 0xFFD4B2EC;
    PAL[0x34] = 0xFFECAEEC;
    PAL[0x35] = 0xFFECAED4;
    PAL[0x36] = 0xFFECB4B0;
    PAL[0x37] = 0xFFE4C490;
    PAL[0x38] = 0xFFCCD278;
    PAL[0x39] = 0xFFB4DE78;
    PAL[0x3A] = 0xFFA8E290;
    PAL[0x3B] = 0xFF98E2B4;
    PAL[0x3C] = 0xFFA0D6E4;
    PAL[0x3D] = 0xFFA0A2A0;
    PAL[0x3E] = 0xFF000000;
    PAL[0x3F] = 0xFF000000;
}

void
ppu_init(struct ppu *ppu)
{
    ppu->vram = malloc(0x4000); // 2kB vram for nametables

    FOR(i, 0, 2) ppu->pattern_tables_pix[i] = malloc(0x1000 * 32); //

    ppu->registers.ppuctrl   = 0b00000000;
    ppu->registers.ppumask   = 0b00000000;
    ppu->registers.ppustatus = 0b10100000;
    ppu->registers.oamaddr   = 0b00000000;
    ppu->registers.ppuscroll = 0b00000000;
    ppu->registers.ppuaddr   = 0b00000000;
    ppu->registers.ppudata   = 0b00000000;
    ppu->registers.oamdata   = 0b00000000;

    ppu->address_latch = 0;

    pal_init(ppu);
}

void
ppu_free(struct ppu *ppu)
{
    free(ppu->vram);
    FOR(i, 0, 2) free(ppu->pattern_tables_pix[i]);
    free(ppu);
}