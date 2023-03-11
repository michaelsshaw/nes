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

#ifndef NES_MAPPER_H_
#define NES_MAPPER_H_

/*! @file mapper.h */

#include <nes.h>
#include <cpu.h>

#define MAP_MODE_CPU 0
#define MAP_MODE_PPU 1

#define MAP_FUNC_HEADER struct nes *nes, u16 addr, u8 *mem, u8 mode
#define MAP_FUNC(name)  u8 *mapcall_##name(MAP_FUNC_HEADER)

#define MAP_DECL(name) nes->mappers[0x##name] = mapcall_##name
#define MAP_CALL(_nes, name, ...)                                              \
    ((mapcall)_nes->mappers[name])(_nes, __VA_ARGS__)

typedef u8 *(*mapcall)(MAP_FUNC_HEADER);

MAP_FUNC(00);
MAP_FUNC(01);

void
mappers_init(struct nes *nes);

#endif // NES_MAPPER_H_
