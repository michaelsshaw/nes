#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <em6502.h>
#include <string.h>

#include <nes.h>
#include "ppu.h"
#include "util.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "mappers/mapper.h"

#define ASPECT (16 / 15)

struct nes *NES;

int echooff = 1;

void
mappers_init()
{
    MAP_DECL(00);
}

void
nes_window_init(SDL_Window **win, SDL_Renderer **ren)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("SDL2",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          1200,
                                          1200 / ASPECT,
                                          SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    *win = window;
    *ren = renderer;
}

void
nes_window_loop(SDL_Window *window, SDL_Renderer *renderer, struct nes *nes)
{
    const int tw = 256;
    const int th = 240;
    const int ts = tw * th;

    u32 *pixels = malloc(ts * 4);

    memset(pixels, 0x00, ts * 4);

    SDL_Texture *texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, tw, th);

    SDL_Event ev;
    int       i = 0;
    for (;;)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                goto out;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, tw * 4);

        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

out:
    free(nes->cpu->mem);
    free(nes->ppu->mem);
    free(nes->cpu);
    free(nes->ppu);
    free(nes->mappers);
    free(nes);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int
main(int argc, char **argv)
{

    NES           = malloc(sizeof(struct nes));
    NES->cpu      = malloc(sizeof(struct cpu));
    NES->ppu      = malloc(sizeof(struct ppu));
    NES->cpu->mem = malloc(0xFFFF);
    NES->ppu->mem = malloc(0x4000);
    NES->mappers  = malloc(0xFF * sizeof(void *));

    struct nes *nes = NES;

    mappers_init();

    SDL_Window   *window;
    SDL_Renderer *renderer;

    nes_window_init(&window, &renderer);
    nes_window_loop(window, renderer, nes);

    if (argc != 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    memset(nes->cpu, 0, sizeof(struct cpu));
    memset(nes->cpu->mem, 0, MEM_SIZE);

    cpu_reset(nes);

    // LOAD ROM

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
        noprintf("File open failed\n");
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

            nes->cartridge.chr = malloc(schr);
            nes->cartridge.prg = malloc(sprg);

            fread(nes->cartridge.prg, sprg, 1, file);
            fread(nes->cartridge.chr, schr, 1, file);

            break;

        default:
            break;
    }

    fclose(file);

    return 0;
}

void
noprintf(char *s, ...)
{
    if (!echooff)
    {
        va_list argptr;
        va_start(argptr, s);
        vprintf(s, argptr);
        va_end(argptr);
    }
}