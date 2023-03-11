CFLAGS := -O2 -w -MMD
CFLAGS += -Iinclude -I./6502/include -I/usr/lib/include/SDL2
LDFLAGS := -lm -lSDL2

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)
DEPS := $(SRC:.c=.d)

OUT := nes
6502 := 6502/lib6502.a

$(OUT): $(OBJ) $(6502)
	@$(CC) $^ $(LDFLAGS) -o $@
	@echo "  LD     $@"

$(6502):
	@make -C 6502

-include $(DEPS)

%.o: %.c
	@$(CC) -c $< $(CFLAGS) -o $@
	@echo "  CC     $@"

.PHONY += clean
clean:
	rm -rf $(OUT) $(DEPS) $(OBJ)
	make -C 6502 clean
