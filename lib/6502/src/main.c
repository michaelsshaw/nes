#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <time.h>

#include <em6502.h>
#include <cpu.h>

struct em6502 *EM;

long long FREQ = 10000000000000L;

int chlast;

int cpu_echooff = 0;

void
noprintf(char *s, ...)
{
    if (!cpu_echooff)
    {
        va_list argptr;
        va_start(argptr, s);
        vprintf(s, argptr);
        va_end(argptr);
    }
}


// MAIN FUNCTION
// generally not used as this is intended as a library
/*int
main(int argc, char **argv)
{

    if (argc != 2)
    {
        noprintf("Usage: 6502 filename\n");
        return 1;
    }
    printf("here!\n");

    EM      = malloc(sizeof(struct em6502));
    EM->cpu = malloc(sizeof(struct cpu));
    EM->cpu->mem = malloc(MEM_SIZE);

    printf("here!\n");

        printf("%d!\n", EM->cpu->mem[0]);

    memset(EM->cpu->mem, 0, MEM_SIZE);
    printf("here!\n");
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL)
    {
        noprintf("File open failed\n");
        return 1;
    }
        printf("here!\n");

    fread(EM->cpu->mem, MEM_SIZE, 1, file);
    fclose(file);
printf("post file\n");

    struct timespec nowt;

    // INIT SETTINGS

    EM->cpu->PC = 0x0400;
    EM->cpu->SP = 0x00;

    // EM->cpu->PC = 0x3364;
    // EM->cpu->SP = 0xFF;
    // EM->cpu->A  = 0x29;
    // EM->cpu->X  = 0x0E;
    // EM->cpu->Y  = 0xFF;  OLD DEBUG STUFF

    // EM->cpu->flags = FLAG_V | FLAG_C;

    // EM->mem[0x0200] = 0x29; // test case

    while (1)
    {

        // if (now - last > nspercycle)
        {
            cpu_clock(EM->cpu);
        }
    }

    free(EM->cpu->mem);
    free(EM->cpu);
    free(EM);

    return 0;
}*/