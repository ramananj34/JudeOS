CC := clang
LD := ld.lld

CFLAGS  := --target=x86_64-unknown-none-elf -ffreestanding -nostdlib -mno-red-zone -fno-stack-protector -Wall -Wextra
LDFLAGS := -T linker.ld

kernel.elf: kmain.o linker.ld
	$(LD) $(LDFLAGS) kmain.o -o kernel.elf

kmain.o: src/kmain.c
	$(CC) $(CFLAGS) -c src/kmain.c -o kmain.o

clean:
	rm -f kmain.o kernel.elf

.PHONY: clean