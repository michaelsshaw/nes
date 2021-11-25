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

#ifndef NES_CPU_H_
#define NES_CPU_H_

/*! @file nescpu.h */

/*!
 * Since the NES stores memory in a number of different places, but maps it
 * all along a 16-bit memory address. This NES memory address needs to be
 * translated into a pointer to a memory array in the NES project. This function
 * is the first in that chain of events. It gets locations that are not affected
 * by a mapper
 *
 * @returns a pointer to the actual memory byte being accessed
 *
 * @param cpu 6502 CPU that is connected to the memory being fetched
 * @param addr 16-bit address to be translated
 *
 */
u8 *
cpu_get_mempointer(struct cpu *cpu, u16 addr);

/*!
 * Override of the default 6502 cpu_read function
 *
 * @param cpu 6502 CPU that is connected to the memory being fetched
 * @param addr 16-bit address to be read
 *
 * @returns 8-bit value stored in memory at addr
 *
 * @see nes_cpu_write()
 * @see cpu_get_mempointer()
 */
u8
nes_cpu_read(struct cpu *cpu, u16 addr);

/*!
 * Override of the default 6502 cpu_write function
 *
 * @param cpu 6502 CPU that is connected to the memory being fetched
 * @param addr 16-bit address to be written to
 * @param val 8-bit value to be written
 *
 * @see nes_cpu_read()
 * @see cpu_get_mempointer()
 */
void
nes_cpu_write(struct cpu *cpu, u16 addr, u8 val);

#endif // NES_CPU_H_
