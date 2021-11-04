#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define FLAG_N 0x80
#define FLAG_V 0x40
#define FLAG_5 0x20
#define FLAG_B 0x10
#define FLAG_D 0x08
#define FLAG_I 0x04
#define FLAG_Z 0x02
#define FLAG_C 0x01

#define RESET_VECTOR 0xFFFC

#define GETFLAG(em, flag) (((em)->flags & (flag)) == (flag))
#define SETFLAG(em, flag) ((em)->flags |= (flag))
#define CLFLAG(em, flag)  ((em)->flags &= (~(flag)))
#define CLFLAGS(em)       ((em)->flags = 0x00)
#define NEG8(num)         ((((num)&0x80)) > 0)

#define SETFLAGC(em, flag, cond)                                               \
    ((cond) ? SETFLAG((em), (flag)) : CLFLAG((em), (flag)))

#define MEM_SIZE 65536 // 64K in bytes

struct nes;

struct cpu
{
    u8  A, X, Y; // registers
    u8  SP;      // stack pointer
    u16 PC;      // program counter
    u8  flags;   // C, Z, (  ), I, D, B, V, N flags;

    u8 *mem;
    u8  cycles;
    u8  addrmode;
};

void
cpu_push(struct nes *em, u8 val);

u8
cpu_pop(struct nes *em);

u8
mem_read(struct nes *em, u16 addr);

void
mem_write(struct nes *em, u16 addr, u8 val);

void
cpu_clock(struct nes *em);

#endif // CPU_H_