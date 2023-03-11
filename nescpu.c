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

#include <stdio.h>

#include <instructions.h>
#include <opcodes.h>

#include <nes.h>
#include "ppu.h"
#include "util.h"

#include "mapper.h"

#define CPU    cpu
#define PC     CPU->PC
#define SP     CPU->SP
#define A      CPU->A
#define X      CPU->X
#define Y      CPU->Y
#define MM     CPU->mem
#define CYCLE  CPU->cycles += 1
#define CYCLES CPU->cycles

#define DBG_FLAG(flag) GETFLAG(CPU, flag) ? #flag[sizeof(#flag) - 2] : '-'

u8
nes_cpu_read(struct cpu *cpu, u16 addr)
{
    struct nes *em = (struct nes *)CPU->fw;
    IFINRANGE(addr, 0x0000, 0x1FFF) //
    {
        return *(MM + (addr & 0x07FF));
    }

    IFINRANGE(addr, 0x2000, 0x3FFF) //
    {
        return ppu_cpu_read(em->ppu, addr);
    }

    if (addr == 0x4016)
    {
        u8 r   = *(MM + addr);
        u8 tmp = (em->btn_latch & 0x80) > 0;
        r &= 0xE0;
        r |= (tmp);
        em->btn_latch <<= 1;
        em->btn_latch |= tmp;

        return r;
    }

    return *MAP_CALL(em, em->cartridge.mapper, addr, MM, MAP_MODE_CPU);
}

void
nes_cpu_write(struct cpu *cpu, u16 addr, u8 val)
{
    u8 *mem = MM;

    struct nes *em = (struct nes *)CPU->fw;
    IFINRANGE(addr, 0x0000, 0x1FFF) //
    {
        mem += (addr & 0x07FF);
    }
    else IFINRANGE(addr, 0x2000, 0x3FFF) //
    {
        ppu_cpu_write(em->ppu, addr, val);
        return;
    }
    else if (addr == 0x4014) // OAM DMA
    {
        // Example:
        // lda $XX
        // sta $4014
        //
        // Copies values of $XXYY-$XXFF to PPU OAM
        // where $YY = OAMADDR

        struct ppu *ppu = em->ppu;

        u16 hi = 0x0000 | val;
        hi <<= 8;
        u16 i;
        for (i = 0; i <= 0xFF; i++)
        {
            ppu->oam[(i + ppu->registers.oamaddr) & 0xFF] = nes_cpu_read(CPU, hi | i);
        }
        
        // Takes a few cycles to do this, so just delay things
        cpu->cycles += 513 + ((em->cycle & 1) == 1 ? 1 : 0);
        return;
    }
    else if (addr == 0x4016 && (val & 0x01) == 0x00)
    {
        em->btn_latch = em->btns;
        return;
    }
    else
    { //
        mem = MAP_CALL(em, em->cartridge.mapper, addr, mem, MAP_MODE_CPU);
    }

    *mem = val;
}
