# This file is in the public domain.

CC = arm-none-eabi-gcc

LIBOPENCM3_DIR ?= ../libopencm3

CFLAGS_LANG = -fno-common -std=c99
CFLAGS_OPT = -Os -flto -fno-fat-lto-objects -pipe
CFLAGS_TARGET = -DSTM32F1 -I$(LIBOPENCM3_DIR)/include -mcpu=cortex-m3 -mfix-cortex-m3-ldrd -msoft-float -mthumb
CFLAGS_WARN = -Wall -Wcast-align -Wextra -Winit-self -Wmissing-include-dirs -Wmissing-prototypes -Wold-style-definition -Wredundant-decls -Wshadow -Wstrict-prototypes -Wundef -Wwrite-strings -pedantic
CFLAGS = $(CFLAGS_LANG) $(CFLAGS_OPT) $(CFLAGS_TARGET) $(CFLAGS_WARN)

LDFLAGS_OPT = -Wl,--as-needed -Wl,--gc-sections -Wl,-O1
LDFLAGS_TARGET = -L$(LIBOPENCM3_DIR)/lib -Tstm32/f1/stm32f103x8.ld -nostdlib -static
LDFLAGS = $(LDFLAGS_OPT) $(LDFLAGS_TARGET)

LDLIBS = -lgcc -lopencm3_stm32f1 -lc

cdc-echo.elf: cdc-echo.o
	$(LINK.c) $^ $(LDLIBS) -o $@
