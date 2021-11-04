#ifndef NES_MAPPER_H_
#define NES_MAPPER_H_

#include <nes.h>
#include <cpu.h>

#define MAP_MODE_CPU 0
#define MAP_MODE_PPU 1

#define MAP_FUNC_HEADER struct nes *nes, u16 addr, u8 *mem, u8 mode

#define MAP_FUNC(name) u8 *mapcall_##name(MAP_FUNC_HEADER)
#define MAP_DECL(name) NES->mappers[0x##name] = mapcall_##name
#define MAP_CALL(_nes, name, ...)                                              \
    ((mapcall)_nes->mappers[name])(_nes, __VA_ARGS__)

typedef u8 *(*mapcall)(MAP_FUNC_HEADER);

MAP_FUNC(00);

#endif // NES_MAPPER_H_