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

#define ASPECT   (16 / 15)
#define ALTN(_n) ((_n) = (_n) == 0 ? 1 : 0)

#define ONE_BILLION           1000000000
#define NES_CPU_CLOCKS_SECOND 1790000
#define NS_CLOCK              ONE_BILLION / NES_CPU_CLOCKS_SECOND
#define NS_FRAME              ONE_BILLION / 60

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

int
nes_game_loop(void *in)
{

    struct nes *nes = (struct nes *)in;

    uint64_t last    = 0;
    uint64_t now     = 0;
    uint64_t start   = 0;
    uint64_t lastlog = 0;

    uint64_t cyclecount = 0;

    for (;;)
    {
        now = nes_time_get();
        if(now - lastlog >= ONE_BILLION)
        {
            printf("Ran %lld cycles\n", cyclecount);
            cyclecount = 0;
            lastlog = now;
        }

        if (now - last < NS_CLOCK)
            continue;

        ppu_clock(nes->ppu);
        ppu_clock(nes->ppu);
        ppu_clock(nes->ppu);

        cpu_clock(nes->cpu);

        last = now;

        cyclecount++;
    }

    return 0;
}

/*!
 *  Main window loop for the entire NES program.

 * @param nes NES data structure for this window
 */
void
nes_window_loop(struct nes *nes)
{
    const int tw = 128;
    const int th = 128;
    const int ts = tw * th;

    /*
     *
     * Initialize the SDL window
     *
     */

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("SDL2",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          1200,
                                          1200 / ASPECT,
                                          SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    /*
     *
     * Create the pixel array, texture and event
     * Fill pixel array with the pattern table
     *
     */
    u32 *pixels = malloc(ts * 4);

    memset(pixels, 0x00, ts * 4);

    SDL_Texture *texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, tw, th);

    SDL_Event ev;
    int       i = 0;

    u8   pt = 0;
    u32 *p2 = ppu_get_patterntable(nes->ppu, pt, 0);
    SDL_UpdateTexture(texture, NULL, p2, tw * 4);

    u32 clockcount = 0;
    u8  pause      = 1;

    SDL_Thread *thread =
      SDL_CreateThread(nes_game_loop, "NES_GAME_LOOP", (void *)nes);

    uint64_t last = 0;
    uint64_t now  = 0;

    for (;;)
    {
        now = nes_time_get();

        if (now - last < NS_FRAME)
            continue;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                goto out;
            }
            if (ev.type == SDL_KEYDOWN)
            {
                switch (ev.key.keysym.sym)
                {
                    case SDLK_a:
                        p2 = ppu_get_patterntable(nes->ppu, ALTN(pt), 0);
                        SDL_UpdateTexture(texture, NULL, p2, tw * 4);

                        break;
                    case SDLK_p:
                        ALTN(pause);
                        break;
                    case SDLK_f:
                        break;
                }
            }
        }

        SDL_RenderCopy(renderer, texture, NULL, NULL);
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
    NES->cpu->mem     = malloc(0xFFFF);
    NES->mappers      = malloc(0xFF * sizeof(void *));
    NES->offs         = 0;
    NES->cpu->echooff = 0;

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

            printf("CHR SIZE: %04X CHR SIZE: %04X\n", sprg, schr);

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
