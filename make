export DEVKITPRO := /opt/devkitpro
export DEVKITARM := $(DEVKITPRO)/devkitARM
export PATH     := $(DEVKITARM)/bin:$(PATH)

PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

ARCH    := -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) -O2 -Wall
LDFLAGS := $(ARCH) -specs=gba.specs

all: main.gba

main.gba: main.elf
	$(OBJCOPY) -O binary main.elf main.gba
	gfix main.gba

main.elf: main.o
	$(CC) main.o $(LDFLAGS) -o main.elf

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm -f *.o *.elf *.gba
