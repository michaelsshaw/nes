#include <stdlib.h>

#include <cpu.h>
#include <em6502.h>

#include "ppu.h"
#include "mapper.h"
#include "util.h"

#define PAL                ppu->pal
#define FOR(_name, _a, _b) for (u16(_name) = (_a); (_name) < (_b); (_name)++)

#define PTSETPIXEL(_pt, _x, _y, _val) (_pt)[(_x) + (_y)*128] = (_val)
#define PCOLREAD(_pal, _pix)                                                   \
    PAL[ppu_read(ppu, 0x3F00 + ((_pal) << 0x02) + _pix) & 0x3F]

#define PPUFLAG(_ppu, _reg, _flag)    ((_ppu->registers._reg & (_flag)) ? 1 : 0)
#define PPUSETFLAG(_ppu, _reg, _mask) _ppu->registers._reg |= (_mask)
#define CLPPUFLAG(_ppu, _reg, _mask)  _ppu->registers._reg &= (~(_mask))

#define RESET_SHIFTERS(_ppu)                                                   \
    _ppu->bg_shift_plo = (_ppu->bg_shift_plo & 0xFF00) | (_ppu->bg_lsb);       \
    _ppu->bg_shift_phi = (_ppu->bg_shift_phi & 0xFF00) | (_ppu->bg_msb);       \
    _ppu->bg_shift_alo =                                                       \
      (_ppu->bg_shift_alo & 0xFF00) | ((_ppu->bg_at & 0x01) ? 0xFF : 0x00);    \
    _ppu->bg_shift_ahi =                                                       \
      (_ppu->bg_shift_ahi & 0xFF00) | ((_ppu->bg_at & 0x02) ? 0xFF : 0x00);

#define UPDATE_SHIFTERS(_ppu)                                                  \
    if (_ppu->registers.ppumask & PPUMASK_b)                                   \
    {                                                                          \
        _ppu->bg_shift_plo <<= 1;                                              \
        _ppu->bg_shift_phi <<= 1;                                              \
        _ppu->bg_shift_alo <<= 1;                                              \
        _ppu->bg_shift_ahi <<= 1;                                              \
    }

#define TAX(_ppu)                                                              \
    if (PPUFLAG(_ppu, ppumask, PPUMASK_b) ||                                   \
        PPUFLAG(_ppu, ppumask, PPUMASK_s))                                     \
    {                                                                          \
        u16 _taxmask = ((PPULOOPY_X | PPULOOPY_CX));                           \
        _ppu->vaddr &= (~_taxmask);                                            \
        _ppu->vaddr |= (_ppu->taddr & _taxmask);                               \
    }

#define TAY(_ppu)                                                              \
    if (PPUFLAG(_ppu, ppumask, PPUMASK_b) ||                                   \
        PPUFLAG(_ppu, ppumask, PPUMASK_s))                                     \
    {                                                                          \
        u16 _taymask = ((PPULOOPY_FY | PPULOOPY_Y | PPULOOPY_CY));             \
        _ppu->vaddr &= (~_taymask);                                            \
        _ppu->vaddr |= (_ppu->taddr & _taymask);                               \
    }

u8
ppu_cpu_read(struct ppu *ppu, u16 addr)
{
    addr = addr & 0x0007;
    addr += 0x2000;

    u8 r = 0x00;
    switch (addr)
    {
        case PPUSTATUS: // $2002
            r = ppu->registers.ppustatus;
            CLPPUFLAG(ppu, ppustatus, PPUSTATUS_V);
            ppu->address_latch = 0;
            break;
        case PPUDATA: // $2007
            r         = ppu->data;
            ppu->data = ppu_read(ppu, ppu->vaddr);
            if (ppu->vaddr >= 0x3F00)
                r = ppu->data;

            ppu->vaddr += PPUFLAG(ppu, ppuctrl, PPUCTRL_I) ? 32 : 1;
            break;
    }

    return r;
}

void
ppu_cpu_write(struct ppu *ppu, u16 addr, u8 data)
{
    addr = addr & 0x0007;
    addr += 0x2000;

    u16 d = 0x0000 | data;

    switch (addr)
    {
        case PPUCTRL: // $2000
            ppu->registers.ppuctrl = data;
            ppu->taddr |= ((d & 0x03) << 10);
            break;
        case PPUMASK: // $2001
            ppu->registers.ppumask = data;
            break;
        case PPUSCROLL: // $2005
            if (ppu->address_latch == 0)
            {
                ppu->fxscroll = data & 0x07;
                ppu->taddr = ppu->taddr | ((data >> 3) & 0x1F); // set coarse x
                ppu->address_latch = 1;
            }
            else
            {
                ppu->taddr |= ((d & 0x07) << 12); // fine y
                ppu->taddr |= ((d & 0xF8) << 2);  // coarse y
                ppu->address_latch = 0;
            }
            break;
        case PPUADDR: // $2006
            if (ppu->address_latch == 0)
            {

                ppu->taddr         = (ppu->taddr & 0x00FF) | ((d & 0x3F) << 8);
                ppu->address_latch = 1;
            }
            else
            {
                ppu->taddr         = (ppu->taddr & 0xFF00) | d;
                ppu->vaddr         = ppu->taddr;
                ppu->address_latch = 0;
            }
            break;
        case PPUDATA: // $2007
            ppu_write(ppu, ppu->vaddr, data);
            ppu->vaddr += PPUFLAG(ppu, ppuctrl, PPUCTRL_I) ? 32 : 1;
            break;
    }
}

u8 *
ppu_get_mempointer(struct ppu *ppu, u16 addr)
{
    u8 *mem = ppu->vram;

    struct nes *em = ppu->fw;

    IFINRANGE(addr, 0x2000, 0x3EFF) // nametable mirroring
    {
        addr &= em->mirror;
    }
    else IFINRANGE(addr, 0x3F00, 0x3FFF) // palette readdressing
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
                    lsb >>= 0x01; // we're reading from
                                  // right to left here
                    msb >>= 0x01; // because of the & 0x01
                                  // operations above printf("
                                  // pix: %d ", pix);
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

static void
ppu_scroll_inc_x(struct ppu *ppu)
{
    if (PPUFLAG(ppu, ppumask, PPUMASK_b) || PPUFLAG(ppu, ppumask, PPUMASK_s))
    {
        if (((ppu->vaddr) & 0x1F) == 31)
        {
            ppu->vaddr = (ppu->vaddr) & (~0x001F);
            ppu->vaddr = (ppu->vaddr) ^ 0x0400;
        }
        else
        {
            ppu->vaddr = ppu->vaddr + 1;
        }
    }
}

static void
ppu_scroll_inc_y(struct ppu *ppu)
{
    if (PPUFLAG(ppu, ppumask, PPUMASK_b) || PPUFLAG(ppu, ppumask, PPUMASK_s))
    {
        if (((ppu->vaddr) & PPULOOPY_FY) != PPULOOPY_FY)
        {
            ppu->vaddr = ppu->vaddr + 0x1000;
        }
        else
        {
            ppu->vaddr = (ppu->vaddr) & (~0x7000);
            u16 cy     = ((ppu->vaddr) & PPULOOPY_CY) >> 5;
            if (cy == 29)
            {
                cy         = 0;
                ppu->vaddr = (ppu->vaddr) ^ PPULOOPY_Y;
            }
            else if (cy == 31)
            {
                cy = 0;
            }
            else
            {
                cy += 1;
            }
            ppu->vaddr = (ppu->vaddr & (~0x03E0)) | (cy << 5);
        }
    }
}
void
ppu_clock(struct ppu *ppu)
{
    int cycle    = ppu->cycle;
    int scanline = ppu->scanline;
    u16 vaddr    = ppu->vaddr;
    IFINRANGE(scanline, -1, 239)
    {
        if (scanline == 0 && cycle == 0)
        {
            ppu->cycle = 1;
            cycle      = 1;
        }

        if (scanline == -1 && cycle == 1)
        {
            CLPPUFLAG(ppu, ppustatus, PPUSTATUS_V);
        }
        u16 tmp;
        if (INRANGE(cycle, 2, 257) || INRANGE(cycle, 321, 337))
        {
            UPDATE_SHIFTERS(ppu);
            switch ((cycle - 1) % 8)
            {
                case 0:
                    RESET_SHIFTERS(ppu);
                    ppu->bg_id = ppu_read(ppu, 0x2000 | (vaddr & 0x0FFF));
                    break;
                case 2:
                    ppu->bg_at = ppu_read(ppu,
                                          0x23C0                      //
                                            | ((vaddr)&0x0C00)        //
                                            | ((vaddr >> 4) & 0x0038) //
                                            | ((vaddr >> 2) & 0x0007));
                    if (vaddr & 0x40)
                        ppu->bg_at >>= 4;
                    if (vaddr & 0x02)
                        ppu->bg_at >>= 2;
                    ppu->bg_at &= 0x03;
                    break;
                case 4:
                    tmp = 0x0000 | (ppu->bg_id);
                    ppu->bg_lsb =
                      ppu_read(ppu,                                     //
                               (PPUFLAG(ppu, ppuctrl, PPUCTRL_B) << 12) //
                                 + (tmp << 4)                           //
                                 + ((vaddr >> 12) & 0x07));
                    break;
                case 6:
                    tmp = 0x0000 | (ppu->bg_id);
                    ppu->bg_msb =
                      ppu_read(ppu,                                     //
                               (PPUFLAG(ppu, ppuctrl, PPUCTRL_B) << 12) //
                                 + (tmp << 4)                           //
                                 + ((vaddr >> 12) & 0x07) + 8);
                    break;
                case 7:
                    ppu_scroll_inc_x(ppu);
                    break;
            }
        }
        if (cycle == 256)
        {
            ppu_scroll_inc_y(ppu);
        }

        if (cycle == 257)
        {
            RESET_SHIFTERS(ppu);
            TAX(ppu);
        }
        // No reason to include this, but i do :^)
        if (cycle == 338 || cycle == 340)
        {
            ppu->bg_id = ppu_read(ppu, 0x2000 | (vaddr & 0x0FFF));
        }

        if (scanline == -1 && INRANGE(cycle, 280, 304))
        {
            TAY(ppu);
        }
    }

    if (scanline == 241 && cycle == 1)
    {
        PPUSETFLAG(ppu, ppustatus, PPUSTATUS_V);
        if (PPUFLAG(ppu, ppuctrl, PPUCTRL_V))
        {
            ppu->nmi = 1;
        }
    }

    u8 bgpix = 0;
    u8 bgpal = 0;
    if (PPUFLAG(ppu, ppumask, PPUMASK_b))
    {
        u16 bit_mux = 0x8000 >> ppu->fxscroll;

        u8 pat0 = (ppu->bg_shift_plo & bit_mux) > 0;
        u8 pat1 = (ppu->bg_shift_phi & bit_mux) > 0;

        u8 pal0 = (ppu->bg_shift_alo & bit_mux) > 0;
        u8 pal1 = (ppu->bg_shift_ahi & bit_mux) > 0;

        bgpix = (pat1 << 1) | pat0;
        bgpal = (pal1 << 1) | pal0;
    }

    struct nes *nes = ppu->fw;
    if (INRANGE((cycle - 1), 0, NES_WIDTH - 1) &&
        INRANGE(scanline, 0, NES_HEIGHT - 1) && 1)
    {
        nes->pixels[(cycle - 1) + scanline * NES_WIDTH] =
          PCOLREAD(bgpal, bgpix);
    }

    ppu->cycle += 1;

    if (ppu->cycle > 340)
    {
        ppu->cycle    = 0;
        ppu->scanline = ppu->scanline + 1;

        if (ppu->scanline >= 261)
        {
            ppu->scanline = -1;
        }
    }
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

    ppu->scanline = -1;
    ppu->cycle    = 30;

    pal_init(ppu);
}

void
ppu_free(struct ppu *ppu)
{
    free(ppu->vram);
    FOR(i, 0, 2) free(ppu->pattern_tables_pix[i]);
    free(ppu);
}
