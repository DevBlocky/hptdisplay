TOOLPREFIX ?= ./toolchain/bin/aarch64-none-elf-
CC = $(TOOLPREFIX)gcc
CFLAGS += -g -Wall -ffreestanding -O2

OBJS=\
	entry.o \
	panic.o \
	switch.o \
	trap.o \
	init.o \
	mmu.o \
	alloc.o \
	intr.o \
	task.o \
	gpio.o \
	uart.o \
	i2c.o \
	bme280.o \
	ssd1306.o \
	bitmap.o

.PHONY: all qemu qemu-gdb gdb clean

all: kernel8.img

kernel8.elf: $(OBJS)
	$(TOOLPREFIX)ld -nostdlib $^ -T kernel8.ld -o $@
kernel8.img: kernel8.elf
	$(TOOLPREFIX)objcopy -O binary $< $@
kernel8.asm: kernel8.elf
	$(TOOLPREFIX)objdump -d $< > $@

QEMU=qemu-system-aarch64
QEMUOPTS=-M raspi3b \
		 -display none \
		 -kernel kernel8.img \
		 -serial stdio
qemu: kernel8.img
	$(QEMU) $(QEMUOPTS)
qemu-gdb: kernel8.img
	@echo "use 'make gdb' in another terminal"
	$(QEMU) $(QEMUOPTS) -S -s
gdb: kernel8.elf
	$(TOOLPREFIX)gdb kernel8.elf

clean:
	@rm -f kernel8.elf kernel8.img kernel8.asm *.o
