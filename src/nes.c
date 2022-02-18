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

/*! @file nes.c
 *
 *
 *
 */

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <em6502.h>
#include <cpu.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include <nes.h>
#include "ppu.h"
#include "util.h"
#include "nescpu.h"
#include "mapper.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#define NES_BTN_A   0x80
#define NES_BTN_B   0x40
#define NES_BTN_SEL 0x20
#define NES_BTN_STR 0x10
#define NES_BTN_UP  0x08
#define NES_BTN_DN  0x04
#define NES_BTN_LF  0x02
#define NES_BTN_RT  0x01

#define ASPECT   (float)(16 / 15)
#define ALTN(_n) ((_n) = (_n) == 0 ? 1 : 0)

#define ONE_BILLION           1000000000
#define NES_CPU_CLOCKS_SECOND (1790000 * 3)
#define NS_CLOCK              ONE_BILLION / NES_CPU_CLOCKS_SECOND
#define NS_FRAME              ONE_BILLION / 60

#define RECT_DECL(_name, _x, _y, _w, _h)                                       \
    struct SDL_Rect nesrect_##_name = {                                        \
        .x = (_x), .y = (_y), .w = (_w), .h = (_h)                             \
    }

// use BUTTON_SET(btnreg, NES_BTN_A, |); to set
// or  BUTTON_SET(btnreg, NES_BTN_A, & ~); to clear
#define BUTTON_SET(_reg, _btn, _op) ((_reg) = ((_reg)_op(_btn)))

// i don't wanna type this twice(once for keyup, once for keydown
#define BUTTON_AUTOSET(_reg, _op)                                              \
    switch (ev.key.keysym.sym)                                                 \
    {                                                                          \
        case SDLK_LEFT:                                                        \
            BUTTON_SET((_reg), NES_BTN_LF, _op);                               \
            break;                                                             \
        case SDLK_RIGHT:                                                       \
            BUTTON_SET((_reg), NES_BTN_RT, _op);                               \
            break;                                                             \
                                                                               \
        case SDLK_UP:                                                          \
            BUTTON_SET((_reg), NES_BTN_UP, _op);                               \
            break;                                                             \
        case SDLK_DOWN:                                                        \
            BUTTON_SET((_reg), NES_BTN_DN, _op);                               \
            break;                                                             \
        case SDLK_BACKSPACE:                                                   \
            BUTTON_SET((_reg), NES_BTN_SEL, _op);                              \
            break;                                                             \
        case SDLK_RETURN:                                                      \
            BUTTON_SET((_reg), NES_BTN_STR, _op);                              \
            break;                                                             \
        case SDLK_z:                                                           \
            BUTTON_SET((_reg), NES_BTN_A, _op);                                \
            break;                                                             \
        case SDLK_x:                                                           \
            BUTTON_SET((_reg), NES_BTN_B, _op);                                \
            break;                                                             \
        case SDLK_SPACE:                                                       \
            BUTTON_SET(nes->btn_speed, 0x01, _op);                             \
        case SDLK_i:                                                           \
            cpu_echooff = cpu_echooff == 0 ? 1 : 0;                            \
    }

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                   \
    (byte & 0x80 ? '1' : '0'), (byte & 0x40 ? '1' : '0'),                      \
      (byte & 0x20 ? '1' : '0'), (byte & 0x10 ? '1' : '0'),                    \
      (byte & 0x08 ? '1' : '0'), (byte & 0x04 ? '1' : '0'),                    \
      (byte & 0x02 ? '1' : '0'), (byte & 0x01 ? '1' : '0')

struct nes *NES;

void
mappers_init();

/*!
 * Returns the current monotonic time in nanoseconds
 *
 * @see nes_window_loop
 */
uint64_t
nes_time_get()
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts))
    {
        fputs("nes_time_get failed!", stderr);
        return 0;
    }
    return 1000000000 * ts.tv_sec + ts.tv_nsec;
}

/*!
 * Infinite loop for running actual CPU, PPU and APU logic
 *
 * Times the loop so that the clock speed of the CPU is 1.79MHz
 *
 * Runs 3 ppu clocks then runs a single CPU clock, as was the timing on the
 * original NES
 *
 * @see ppu_clock
 *
 * @param in Void pointer to a struct nes
 */
int
nes_game_loop(void *in)
{
    struct nes *nes = (struct nes *)in;

    uint64_t last = 0;
    uint64_t now  = 0;

    u8  enable = 0;
    u32 count  = 0;

    while (nes->enable)
    {
        now = nes_time_get();

        if (now - last < NS_CLOCK && !nes->btn_speed) continue;

        ppu_clock(nes->ppu);

        if (nes->ppu->nmi)
        {
            nes->ppu->nmi = 0;
            cpu_nmi(nes->cpu);
            enable = 1;

            count = 0;
        }
        if (nes->cycle % 3 == 0)
        {
            count += 1;
            cpu_clock(nes->cpu);
        }

        if (enable && (nes->ppu->registers.ppustatus & 0x80) == 0)
        {
            enable = 0;
            // printf("%d\n", count);
        }

        nes->cycle += 1;
        last = now;
    }

    return 0;
}

/*!
 * Main window loop for the entire NES program.
 *
 * Creates a second thread that runs a concurrent infinite loop in
 * #nes_game_loop
 *
 * @param nes NES data structure for this window
 */
void
nes_window_loop(struct nes *nes)
{
    float SCALE         = 2.9f;
    float WINDOW_HEIGHT = (float)NES_HEIGHT * SCALE;
    float NES_OUT_WIDTH = (float)NES_WIDTH * SCALE;
    int   WINDOW_WIDTH  = NES_OUT_WIDTH;
    /*
     *
     * Initialize the SDL window
     *
     */

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("mnem",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          WINDOW_WIDTH,
                                          WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Event ev;

    /*
     *
     * IMPORTANT
     *
     * Pixel and SDL_Texture data
     *
     * IMPORTANT
     *
     */

    SDL_Texture *tex_screen = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_ARGB8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                NES_WIDTH,
                                                NES_HEIGHT);

    RECT_DECL(screen, 0, 0, NES_OUT_WIDTH, WINDOW_HEIGHT);
    /*
     * Second thread for actual NES operation
     * The current thread is for SDL only
     */

    SDL_CreateThread(nes_game_loop, "NES_GAME_LOOP", (void *)nes);

    //
    // Infinite loop timing
    //

    uint64_t last = 0;
    uint64_t now  = 0;

    // init the buttons
    nes->btns      = 0x00;
    nes->btn_latch = 0x00;

    for (;;)
    {
        now = nes_time_get();

        SDL_SetRenderDrawColor(renderer, 80, 190, 190, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                nes->enable = 0;
                goto out;
            }
            if (ev.type == SDL_KEYDOWN)
            {
                BUTTON_AUTOSET(nes->btns, |);
            }
            if (ev.type == SDL_KEYUP)
            {
                BUTTON_AUTOSET(nes->btns, &~);
            }
        }

        /*u8 ntx, nty;
        for (int y = 0; y < NES_HEIGHT; y++)
        {
            for (int x = 0; x < NES_WIDTH; x++)
            {
                ntx        = x / 8;
                nty        = y / 8;

                u16 ntaddr = 0x2000 + ntx + nty * 32;
                u8  pix    = ppu_read(nes->ppu, ntaddr);

                u8  pvl    = ppu_read(nes->ppu, (pix << 4) + (y % 8));
                u8  pvh    = ppu_read(nes->ppu, (pix << 4) + (y % 8) +
        0x08);

                pvl >>= (7 - x % 8);
                pvh >>= (7 - x % 8);

                u8 pv  = ((pvl & 0x01) | ((pvh << 1) & 0x02));
                u8 col = ppu_read(nes->ppu, 0x3F00 + pv);
                nes->pixels[x + y * NES_WIDTH] = nes->ppu->pal[col];
            }
        }*/

        //
        // Update the PT pixels and textures
        //

        if (nes->frame_complete)
            SDL_UpdateTexture(tex_screen, NULL, nes->pixels, NES_WIDTH * 4);

        //
        // Copy the textures to the renderer
        //

        SDL_RenderCopy(renderer, tex_screen, NULL, &nesrect_screen);

        SDL_RenderPresent(renderer);

        last = now;
    }

out:
    ppu_free(nes->ppu);
    free(nes->cpu->mem);
    free(nes->cpu);
    free(nes->mappers);
    free(nes);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/*!
 * Entrypoint for the NES emulator
 *
 * Branches to #nes_window_loop
 */
int
main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // *******************
    // EMULATOR & CPU INIT
    // *******************
    NES               = malloc(sizeof(struct nes));
    NES->cpu          = malloc(sizeof(struct cpu));
    NES->ppu          = malloc(sizeof(struct ppu));
    NES->cpu->mem     = malloc(MEM_SIZE);
    NES->mappers      = malloc(0xFF * sizeof(void *));
    NES->offs         = 0;
    NES->cpu->echooff = 0;
    NES->enable       = 1;
    NES->cycle        = 0;

    struct nes *nes = NES;

    nes->cpu->cpu_read  = nes_cpu_read;
    nes->cpu->cpu_write = nes_cpu_write;

    nes->cpu->fw = nes;
    nes->ppu->fw = nes;

    memset(nes->cpu->mem, 0, MEM_SIZE);

    mappers_init();
    ppu_init(nes->ppu);

    // ********
    // LOAD ROM
    // ********

    struct
    {
        u8 name[4]; // useless
        u8 s_prg_rom_16;
        u8 s_chr_rom_8;
        u8 flags6;
        u8 flags7;
        u8 flags8;
        u8 flags9;
        u8 flags10;
        u8 padding[5];
    } header_rom;

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL)
    {
        printf("File open failed\n");
        return 1;
    }

    // read INES header
    fread(&header_rom, 16, 1, file);
    nes->cartridge.mapper =
      (header_rom.flags7 & 0xF0) | (header_rom.flags6 >> 0x04);
    nes->mirror =
      (header_rom.flags6 & 0x01) > 0 ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;
    printf("%d\n", nes->cartridge.mapper);

    if (header_rom.flags6 & 0x04)
    {
        fseek(file, 512, SEEK_CUR); // skip trainer
    }

    int filetype = 1;

    switch (filetype)
    {
        case 1:

            nes->cartridge.s_prg_rom_16 = header_rom.s_prg_rom_16;
            nes->cartridge.s_chr_rom_8  = header_rom.s_chr_rom_8;

            int sprg = nes->cartridge.s_prg_rom_16 * 0x4000;
            int schr = nes->cartridge.s_chr_rom_8 * 0x2000;

            nes->cartridge.chr = malloc(schr);
            nes->cartridge.prg = malloc(sprg);

            fread(nes->cartridge.prg, sprg, 1, file);
            fread(nes->cartridge.chr, schr, 1, file);

            cpu_reset(nes->cpu);

            break;

        default:
            break;
    }

    fclose(file);

    // ************
    // START WINDOW
    // ************
    nes_window_loop(nes);

    return 0;
}
