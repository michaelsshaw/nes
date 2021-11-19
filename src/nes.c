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

#define RECT_DECL(_name, _x, _y, _w, _h)                                       \
    struct SDL_Rect nesrect_##_name = {                                        \
        .x = (_x), .y = (_y), .w = (_w), .h = (_h)                             \
    }

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

    while (nes->enable)
    {
        now = nes_time_get();

        if (now - last < NS_CLOCK)
            continue;

        ppu_clock(nes->ppu);
        ppu_clock(nes->ppu);
        ppu_clock(nes->ppu);

        if (nes->ppu->nmi)
        {
            nes->ppu->nmi = 0;
            cpu_nmi(nes->cpu);
        }
        cpu_clock(nes->cpu);

        last = now;

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
    int       padding = 20;
    const int w_pt    = 128;
    const int h_pt    = 128;
    const int ts      = w_pt * h_pt;

    int WINDOW_HEIGHT = 900;
    int NES_OUT_WIDTH = WINDOW_HEIGHT * ASPECT;
    int WINDOW_WIDTH  = NES_OUT_WIDTH //
                       + padding      // nesview padding;
                       + w_pt         // width of pattern table 1
                       + padding      // padding after pt1
                       + w_pt         // width of pt2
                       + padding;     // padding after pt2

    /*
     *
     * Initialize the SDL window
     *
     */

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("SDL2",
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
    u32 *pixels = malloc(ts * 4);
    memset(pixels, 0x00, ts * 4);

    //
    // First Pattern Table
    //
    SDL_Texture *tex_pt0 = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             w_pt,
                                             h_pt);

    u32 *pix_pt0;
    RECT_DECL(pt0,
              NES_OUT_WIDTH + padding,        //
              WINDOW_HEIGHT - h_pt - padding, //
              w_pt,                           //
              h_pt);                          //

    SDL_Texture *tex_pt1 = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             w_pt,
                                             h_pt);

    u32 *pix_pt1;
    RECT_DECL(pt1,
              WINDOW_WIDTH - w_pt - padding,  //
              WINDOW_HEIGHT - h_pt - padding, //
              w_pt,                           //
              h_pt);                          //

    SDL_Texture *tex_screen = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_ARGB8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                NES_WIDTH,
                                                NES_HEIGHT);

    RECT_DECL(screen, 0, 0, NES_OUT_WIDTH, WINDOW_HEIGHT);
    /**
    // Second thread for actual NES operation
    // The current thread is for SDL only
    **/

    SDL_Thread *thread =
      SDL_CreateThread(nes_game_loop, "NES_GAME_LOOP", (void *)nes);

    //
    // Infinite loop timing
    //

    uint64_t last = 0;
    uint64_t now  = 0;

    for (;;)
    {
        now = nes_time_get();

        if (now - last < NS_CLOCK)
            continue;

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
                switch (ev.key.keysym.sym)
                {
                    case SDLK_f:
                        break;
                }
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
                u8  pvh    = ppu_read(nes->ppu, (pix << 4) + (y % 8) + 0x08);

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

        pix_pt0 = ppu_get_patterntable(nes->ppu, 0, 0);
        SDL_UpdateTexture(tex_pt0, NULL, pix_pt0, w_pt * 4);

        pix_pt1 = ppu_get_patterntable(nes->ppu, 1, 0);
        SDL_UpdateTexture(tex_pt1, NULL, pix_pt1, w_pt * 4);

        SDL_UpdateTexture(tex_screen, NULL, nes->pixels, NES_WIDTH * 4);

        //
        // Copy the textures to the renderer
        //

        SDL_RenderCopy(renderer, tex_pt0, NULL, &nesrect_pt0);
        SDL_RenderCopy(renderer, tex_pt1, NULL, &nesrect_pt1);
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
      (header_rom.flags6 & 0x01) ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;

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

            printf("PRG SIZE: %04X CHR SIZE: %04X\n", sprg, schr);

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
