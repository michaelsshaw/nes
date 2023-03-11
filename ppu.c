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

#include <stdlib.h>
#include <string.h>

#include <cpu.h>
#include <em6502.h>

#include "ppu.h"
#include "mapper.h"
#include "util.h"

#define PAL                ppu->pal
#define FOR(_name, _a, _b) for (u16(_name) = (_a); (_name) < (_b); (_name)++)

#define PTSETPIXEL(_pt, _x, _y, _val) (_pt)[(_x) + (_y)*128] = (_val)
#define PCOLREAD(_pal, _pix)                                                   \
    PAL[ppu_read(ppu, 0x3F00 + ((_pal) << 0x02) + _pix)]

#define PPUFLAG(_ppu, _reg, _flag)    ((_ppu->registers._reg & (_flag)) ? 1 : 0)
#define PPUSETFLAG(_ppu, _reg, _mask) _ppu->registers._reg |= (_mask)
#define CLPPUFLAG(_ppu, _reg, _mask)  _ppu->registers._reg &= (~(_mask))

#define BYTE_FLIP(_i)                                                          \
    _i = (_i & 0xF0) >> 4 | (_i & 0x0F) << 4;                                  \
    _i = (_i & 0xCC) >> 2 | (_i & 0x33) << 2;                                  \
    _i = (_i & 0xAA) >> 1 | (_i & 0x55) << 1;

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
        case OAMDATA: // $2004
            r = ppu->oam[ppu->registers.oamaddr];
            break;
        case PPUDATA: // $2007
            if (ppu->vaddr >= 0x3F00)
            {
                ppu->data = ppu_read(ppu, ppu->vaddr - 0x1000);
                r         = ppu_read(ppu, ppu->vaddr);
            }
            else
            {
                r         = ppu->data;
                ppu->data = ppu_read(ppu, ppu->vaddr);
            }

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
            ppu->registers.ppuctrl = data & 0xFC;
            // forgetting this caused me major headache, and slowed this project
            // down by weeks
            // clear nametable select
            ppu->taddr &= (~0x0C00);
            ppu->taddr |= ((d & 0x03) << 10);
            break;
        case PPUMASK: // $2001
            ppu->registers.ppumask = data;
            break;
        case OAMADDR: // $2003
            ppu->registers.oamaddr = data;
            break;
        case OAMDATA: // $2004
            ppu->oam[ppu->registers.oamaddr] = data;
            ppu->registers.oamaddr += 1;
            break;
        case PPUSCROLL: // $2005
            if (ppu->address_latch == 0)
            {
                ppu->fxscroll = data & 0x07;
                ppu->taddr &= (~PPULOOPY_CX);              // clear cx
                ppu->taddr |= ((data >> 3) & PPULOOPY_CX); // set coarse x
                ppu->address_latch = 1;
            }
            else
            {
                // clear fy, cy and set
                ppu->taddr &= (~(PPULOOPY_CY | PPULOOPY_FY));
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
                ppu->taddr         = (ppu->taddr & 0xFF00) | data;
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
        if (taddr == 0x0010) taddr = 0x0000;
        if (taddr == 0x0014) taddr = 0x0004;
        if (taddr == 0x0018) taddr = 0x0008;
        if (taddr == 0x001C) taddr = 0x000C;

        addr = 0x3F00 + taddr;
        return (mem + addr);

        // printf("%04X ", addr);
    }

    return MAP_CALL(em, em->cartridge.mapper, addr, mem, MAP_MODE_PPU);
}

u8
ppu_read(struct ppu *ppu, u16 addr)
{
    u8 r = *ppu_get_mempointer(ppu, addr);
    IFINRANGE(addr, 0x3F00, 0x3FFF)
    {
        r = r & (PPUFLAG(ppu, ppumask, PPUMASK_G) ? 0x30 : 0x3F);
    }
    return r;
}

void
ppu_write(struct ppu *ppu, u16 addr, u8 val)
{
    *ppu_get_mempointer(ppu, addr) = val;
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

    pal &= 0x07;

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

static inline void
ppu_reset_shifters(struct ppu *ppu)
{
    {
        ppu->bg_shift_plo = (ppu->bg_shift_plo & 0xFF00) | (ppu->bg_lsb);
        ppu->bg_shift_phi = (ppu->bg_shift_phi & 0xFF00) | (ppu->bg_msb);
        ppu->bg_shift_alo = (ppu->bg_shift_alo & 0xFF00) |
                            ((ppu->bg_at & 0x01) > 0 ? 0xFF : 0x00);
        ppu->bg_shift_ahi = (ppu->bg_shift_ahi & 0xFF00) |
                            ((ppu->bg_at & 0x02) > 0 ? 0xFF : 0x00);
    }
}

static inline void
ppu_update_shifters(struct ppu *ppu)
{
    if (PPUFLAG(ppu, ppumask, PPUMASK_b))
    {
        ppu->bg_shift_plo <<= 1;
        ppu->bg_shift_phi <<= 1;
        ppu->bg_shift_alo <<= 1;
        ppu->bg_shift_ahi <<= 1;
    }
}

static inline void
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

static inline void
transfer_address_x(struct ppu *ppu)

{
    if (PPUFLAG(ppu, ppumask, PPUMASK_b) || PPUFLAG(ppu, ppumask, PPUMASK_s))
    {
        u16 taxmask = ((PPULOOPY_X | PPULOOPY_CX));
        ppu->vaddr &= (~taxmask);
        ppu->vaddr |= (ppu->taddr & taxmask);
    }
}

static inline void
transfer_address_y(struct ppu *ppu)
{
    if (PPUFLAG(ppu, ppumask, PPUMASK_b) || PPUFLAG(ppu, ppumask, PPUMASK_s))
    {
        u16 taymask = ((PPULOOPY_FY | PPULOOPY_Y | PPULOOPY_CY));
        ppu->vaddr &= (~taymask);
        ppu->vaddr |= (ppu->taddr & taymask);
    }
}

static inline void
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
            ppu->vaddr &= ~(0x7000);
            u16 cy = ((ppu->vaddr) & PPULOOPY_CY) >> 5;
            if (cy == 29)
            {
                cy = 0;
                ppu->vaddr ^= PPULOOPY_Y;
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

static inline void
ppu_clock_background(struct ppu *ppu)
{
    struct nes *nes = ppu->fw;
    /*
     * Background rendering ONLY
     */

    if (ppu->scanline == -1 && ppu->cycle == 1)
    {
        CLPPUFLAG(ppu, ppustatus, PPUSTATUS_V);
        CLPPUFLAG(ppu, ppustatus, PPUSTATUS_S);
        CLPPUFLAG(ppu, ppustatus, PPUSTATUS_O);

        nes->frame_complete = 0;

        memset(ppu->sp_shift_lo, 0, 16);
    }
    if (INRANGE(ppu->cycle, 1, 256) || INRANGE(ppu->cycle, 321, 336))
    {
        u16 vaddr = ppu->vaddr & 0x7FFF;
        u16 tmp;
        ppu_update_shifters(ppu);
        switch ((ppu->cycle - 1) & 0x7)
        {
            case 0:
                ppu_reset_shifters(ppu);
                ppu->bg_id = ppu_read(ppu, 0x2000 | (vaddr & 0x0FFF));
                break;
            case 2:
                if (PPUFLAG(ppu, ppumask, PPUMASK_b))
                {
                    // attributes start at $23C0 on the nametable,
                    // then we select the nametable (0000 or 0C00 and so
                    // on..) the coarse y ((vaddr >> 4) & 0x38) high 3 bits
                    // and the coarse x ((vaddr >> 2) & 0x07) high 3 bits
                    //
                    // V:     YYYyyXXXxx
                    // Addr >   00YYYXXX
                    u16 addr_att = 0x23C0 | (vaddr & 0x0C00) |
                                   ((vaddr >> 4) & 0x0038) |
                                   ((vaddr >> 2) & 0x0007);

                    ppu->bg_at = ppu_read(ppu, addr_att);

                    u8 shift = ((vaddr >> 4) & 0x04) | (vaddr & 0x02);
                    ppu->bg_at >>= shift;
                    ppu->bg_at &= 0x03;
                }
                break;
            case 4:
                tmp = 0x0000 | (ppu->bg_id);
                ppu->bg_lsb =
                  ppu_read(ppu,                                          //
                           ((u16)PPUFLAG(ppu, ppuctrl, PPUCTRL_B) << 12) //
                             + (tmp << 4)                                //
                             + ((vaddr >> 12) & 0x07));
                break;
            case 6:
                tmp = 0x0000 | (ppu->bg_id);
                ppu->bg_msb =
                  ppu_read(ppu,                                          //
                           ((u16)PPUFLAG(ppu, ppuctrl, PPUCTRL_B) << 12) //
                             + (tmp << 4)                                //
                             + ((vaddr >> 12) & 0x07) + 8);
                break;
            case 7:
                ppu_scroll_inc_x(ppu);
                break;
        }
        if (ppu->cycle == 256)
        {
            ppu_scroll_inc_y(ppu);
        }
    }

    if (ppu->cycle == 257)
    {
        ppu_reset_shifters(ppu);
        transfer_address_x(ppu);
    }

    if (ppu->cycle == 338 || ppu->cycle == 340)
    {
        ppu->bg_id = ppu_read(ppu, 0x2000 | (ppu->vaddr & 0x0FFF));
    }

    if (ppu->scanline == -1 && INRANGE(ppu->cycle, 280, 304))
    {
        transfer_address_y(ppu);
    }
}

static inline void
ppu_clock_foreground(struct ppu *ppu)
{
    /*
     * S-OAM init to $FF
     *
     * Technically this takes 64 ppu->cycles to complete
     * because of the memory write speeds, but the computer
     * running this program isn't a NES so....
     */

    if (ppu->cycle == 1)
    {
        memset(ppu->soam, 0xFF, 32);
        ppu->n_oam              = 0;
        ppu->m_oam              = 0;
        ppu->i_soam             = 0;
        ppu->soam_true          = 0;
        ppu->soam_write_disable = 0;
        ppu->sprite_count       = 0;
    }

    /*
     * SPRITE EVALUATION (for the next line)
     */

    IFINRANGE(ppu->cycle, 65, 256)
    {
        u16 tcycle = ppu->cycle - 64;

        if (tcycle & 0x0001) // read on odd cycles
        {
            // this is the data we will either write to S-OAM or pass on
            // next ppu->cycle(because we only write on even cycles)
            ppu->soam_latch = ppu->oam[ppu->n_oam * 4 + ppu->m_oam];
            int16_t diff = ((int16_t)ppu->scanline - (int16_t)ppu->soam_latch);
            if (ppu->m_oam == 0)
            {
                // tell the ppu to write to the soam next 7 ppu->cycles
                // if the sprite is in range
                IFINRANGE(diff, 0, 7 + 8 * PPUFLAG(ppu, ppuctrl, PPUCTRL_H))
                { //
                    ppu->soam_true = 1;
                    if (ppu->n_oam == 0)
                    {
                        ppu->inc_sprite0 = 1;
                    }
                }
                else
                {
                    // if we read a y position(m == 0) and it isn't in
                    // range tell the ppu not to write next
                    ppu->soam_true = 0;
                }
            }
        }
        else // write on even ppu->cycles
        {
            u8 y = ppu->soam_latch;

            if (ppu->soam_true || ppu->m_oam == 0) // always write the y value
            {
                if (ppu->soam_write_disable == 0)
                {
                    ppu->soam[ppu->i_soam * 4 + ppu->m_oam] = y;
                    ppu->m_oam += 1;
                    // if we finished reading the 4 bytes, go back and
                    // increment n
                    if (ppu->m_oam > 3)
                    {
                        ppu->m_oam = 0;
                        ppu->n_oam += 1;

                        ppu->i_soam += 1;
                        ppu->soam_true = 0;
                    }
                }
                else
                {
                    PPUSETFLAG(ppu, ppustatus, PPUSTATUS_O);
                }
            }
            else
            {
                ppu->n_oam += 1;
                ppu->m_oam = 0;
            }
        }

        // Post read-write next-step evaluation

        if (ppu->i_soam == 8) // we found 8 sprites
        {
            ppu->soam_write_disable = 1;
            ppu->soam_true          = 0;
        }
    }

    /*
     * SPRITE FETCHING (for the next line)
     *
     * Use the information we gathered in evaluation and feed it into shift
     * registers that we can actually use to render data
     */

    IFINRANGE(ppu->cycle, 257, 320) //
    {
        // 8 sprites total, 8 ppu->cycles per sprite
        u16 tcycle = ppu->cycle - 257; // 0-63

        // since the temp variables aren't used anymore (i_soam, n_oam,
        // m_oam) we are safe to use them here

        u8  index  = ((u8)(tcycle / 2)); // which part of soam mem(0-31)
        u8  sindex = ((u8)(tcycle / 8)); // which sprite we're on(0-7)
        u16 addr;
        u8  attr;

        u8 rm = index % 4;
        if ((tcycle & 0x01) == 0) // read on even, write on odd, yes it's
                                  // the opposite of the above
        {
            // read pattern data
            u8 out = 0x00;

            switch (rm)
            {
                case 0:
                    out = ppu->soam[sindex * 4 + 2];
                    break;
                case 1:
                    out = ppu->soam[sindex * 4 + 3];
                    break;
                case 2:
                case 3:
                    attr = (ppu->soam[sindex * 4 + 2]);
                    u16 ypos = 0x0000 | (ppu->scanline - ppu->soam[sindex * 4]);

                    // vertical sprite mirroring
                    u8 h = PPUFLAG(ppu, ppuctrl, PPUCTRL_H);

                    if (attr & 0x80) ypos = 7 - ypos;
                    if (h)
                    {
                        addr     = ppu->soam[sindex * 4 + 1];
                        u16 bank = (addr & 0x01) << 12;
                        addr &= 0xFE;
                        addr += (rm - 1);
                        addr <<= 4;
                        addr |= ypos;
                        addr |= bank;
                    }
                    else
                    {
                        addr = //
                          ((ppu->soam[sindex * 4 + 1]
                            << 4) // which sprite in the table
                           | (PPUFLAG(ppu, ppuctrl, PPUCTRL_S) << 12)) +
                          (rm - 2) * 8;

                        addr |= ypos;
                    }

                    out = ppu_read(ppu, addr);
                    // horizontal sprite mirroring
                    if (attr & 0x40)
                    {
                        BYTE_FLIP(out);
                    }
                    break;
            }

            ppu->soam_latch = out;
        }
        else // we're writing on odd ppu->cycles
        {
            u8 *loc = (u8 *)(&ppu->sp_latch);
            loc += rm * 8;
            loc += sindex;

            *loc = ppu->soam_latch;
        }
    }
}

void
ppu_clock(struct ppu *ppu)
{
    struct nes *nes = ppu->fw;

    /* https://www.nesdev.org/wiki/PPU_frame_timing */
    if (ppu->scanline == -1 && ppu->cycle == 0 && ppu->odd_frame)
    {
        ppu->cycle += 1;
    }

    IFINRANGE(ppu->scanline, -1, 239) { ppu_clock_background(ppu); }

    IFINRANGE(ppu->scanline, 0, 239) { ppu_clock_foreground(ppu); }

    /*
     * Cause a CPU NMI if NMI enable flag is 1
     */
    if (ppu->scanline == 241 && ppu->cycle == 1)
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

    if (bgpix == 0) bgpal = 0;

    if (PPUFLAG(ppu, ppumask, PPUMASK_s))
    {
        for (u8 i = 0; i < 8; i++)
        {
            if (ppu->sp_counter[i] == 0)
            // if it is at current x and has priority
            {
                u8 lo = ppu->sp_shift_lo[i];
                u8 hi = ppu->sp_shift_hi[i];

                u8 pat0 = (lo & 0x80) > 0;
                u8 pat1 = (hi & 0x80) > 0;

                u8 pal = (ppu->sp_latch[i] & 0x03) + 4;

                u8 pix = (pat1 << 1) | pat0;

                // we only output a sprite pixel given the following conditions:
                //
                // 1. the sprite pixel is not zero AND one of the following:
                //
                // 2. the background pixel is zero
                // 3. the sprite has priority(0 indicates priority) over the
                //    background
                //
                // sorry for overexplaining this statement :^)
                if (pix > 0 && bgpix > 0 && i == 0 && ppu->inc_sprite0 //
                    && !INRANGE(ppu->cycle, 1, 8) &&
                    PPUFLAG(ppu, ppumask, PPUMASK_b))
                {
                    if ((INRANGE(ppu->cycle, 9, 257) &&
                         !(ppu->registers.ppumask & 0x06)) || //
                        (INRANGE(ppu->cycle, 1, 258) &&
                         (ppu->registers.ppumask & 0x06)))
                    {
                        PPUSETFLAG(ppu, ppustatus, PPUSTATUS_S); // set sprite 0
                                                                 //
                    }
                }
                if (pix > 0 &&
                    (bgpix == 0 || ((ppu->sp_latch[i] & 0x20) == 0)) &&
                    ppu->cycle < 256)
                {
                    bgpix = pix;
                    bgpal = pal;
                }
            }
        }
    }

    if (INRANGE((ppu->cycle - 1), 0, NES_WIDTH - 1) &&
        INRANGE(ppu->scanline, 0, NES_HEIGHT - 1))
    {
        u32 pix = PCOLREAD(bgpal, bgpix);
        nes->pixels[(ppu->cycle - 1) + ppu->scanline * NES_WIDTH] = pix;
    }

    if (ppu->cycle == NES_WIDTH - 1 && ppu->scanline == NES_HEIGHT - 1)
    {
        nes->frame_complete = 1;
    }

    IFINRANGE(ppu->cycle, 1, 256)
    {
        for (u8 i = 0; i < 8; i++)
        {
            if (ppu->sp_counter[i] > 0)
            {
                ppu->sp_counter[i] -= 1;
            }
            else
            {
                ppu->sp_shift_lo[i] <<= 1;
                ppu->sp_shift_hi[i] <<= 1;
            }
        }
    }

    ppu->cycle += 1;

    if (ppu->cycle > 340)
    {
        ppu->odd_frame ^= 0x01;
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

    ppu->scanline = 0;
    ppu->cycle    = 0;

    ppu->odd_frame = 0;

    ppu->bg_at        = 0;
    ppu->bg_id        = 0;
    ppu->bg_lsb       = 0;
    ppu->bg_msb       = 0;
    ppu->bg_shift_plo = 0;
    ppu->bg_shift_phi = 0;
    ppu->bg_shift_alo = 0;
    ppu->bg_shift_ahi = 0;

    pal_init(ppu);
}

void
ppu_free(struct ppu *ppu)
{
    free(ppu->vram);
    FOR(i, 0, 2) free(ppu->pattern_tables_pix[i]);
    free(ppu);
}
