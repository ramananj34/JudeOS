CC := clang
LD := ld.lld
ASM := nasm

CFLAGS  := --target=x86_64-unknown-none-elf -ffreestanding -nostdlib -mno-red-zone -fno-stack-protector -Wall -Wextra
LDFLAGS := -T kernel/linker.ld
BUILD := build
HEADERS := $(wildcard kernel/*.h)
KOBJS := $(BUILD)/entry.o $(BUILD)/kmain.o $(BUILD)/console.o $(BUILD)/gdt.o $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/interrupts.o $(BUILD)/timer.o $(BUILD)/pmm.o $(BUILD)/vmm.o

all: $(BUILD)/os.img

$(BUILD)/bootstage1.bin: boot/bootstage1.asm | $(BUILD)
	$(ASM) -f bin $< -o $@

$(BUILD)/bootstage2.bin: boot/bootstage2.asm | $(BUILD)
	$(ASM) -f bin $< -o $@

$(BUILD)/%.o: kernel/%.asm | $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/%.o: kernel/%.c $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.elf: $(KOBJS) kernel/linker.ld
	$(LD) $(LDFLAGS) $(KOBJS) -o $@

$(BUILD)/os.img: $(BUILD)/bootstage1.bin $(BUILD)/bootstage2.bin $(BUILD)/kernel.elf
	cat $(BUILD)/bootstage1.bin $(BUILD)/bootstage2.bin $(BUILD)/kernel.elf > $@
	truncate -s 131072 $@

$(BUILD):
	mkdir -p $(BUILD)

run: $(BUILD)/os.img
	qemu-system-x86_64 -drive format=raw,file=$(BUILD)/os.img -m 512M -nographic

clean:
	rm -rf $(BUILD)

.PHONY: all run clean