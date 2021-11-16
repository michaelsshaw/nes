#include <stdio.h>

#include <instructions.h>
#include <opcodes.h>

#include <nes.h>
#include "ppu.h"

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

#define INRANGE(_n, _a, _b)   (((_n) >= (_a)) && ((_n) <= (_b)))
#define IFINRANGE(_m, _c, _d) if (INRANGE((_m), (_c), (_d)))

long long cycles = 0;

// u8 *
// cpu_get_mempointer(struct cpu *cpu, u16 addr)
// {
//     u8 *mem = MM;

//     struct nes *em = (struct nes *)cpu->fw;
//     IFINRANGE(addr, 0x0000, 0x1FFF) //
//     {
//         return (mem + (addr & 0x07FF));
//     }

//     IFINRANGE(addr, 0x2000, 0x3FFF) //
//     {
//         u8 *p     = (u8 *)&em->ppu->registers;
//         u8  touch = ppu_touch(em, addr, 1, *mem);
//         p += (addr & 0x0007);
//         return p;
//     }

//     return MAP_CALL(em, em->cartridge.mapper, addr, mem, MAP_MODE_CPU);
// }

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
    else
    {
        mem = MAP_CALL(em, em->cartridge.mapper, addr, mem, MAP_MODE_CPU);
    }

    *mem = val;
}