C_SOURCES = $(wildcard kernel/*.c lib/*.c drivers/*.c utils/*.c)
HEADERS = $(wildcard kernel/*.h lib/*.h drivers/*.h utils/*.h)
OBJ = ${C_SOURCES:.c=.o}
INCLUDE= -I ./kernel -I ./lib -I ./drivers -I ./utils

All: chaos.img

chaos.img:	boot.bin kernel.bin
	dd if=/dev/zero of=chaos.img ibs=1k count=1440
	dd if=boot.bin of=chaos.img conv=notrunc
	dd if=kernel.bin of=chaos.img seek=1 conv=notrunc	

boot.bin:	boot/boot.asm boot/bios.asm boot/pm_gdt.asm boot/pm_start.asm boot/pm_io.asm
	nasm -f bin -o boot.bin boot/boot.asm

%.o : %.c ${HEADERS}
	gcc -m32 -std=gnu99 -ffreestanding $(INCLUDE) -c $< -o $@

kernel/kernel_entry.o: kernel/kernel_entry.asm
	nasm kernel/kernel_entry.asm -f elf -o kernel/kernel_entry.o

kernel.bin: kernel/kernel_entry.o ${OBJ}
	ld -m elf_i386 -o kernel.bin -Ttext 0x1000 $^ --oformat binary

clean:
	rm -rf chaos.img
	rm -rf boot.bin
	rm -rf kernel.bin
	rm -rf kernel/*.o
	rm -rf lib/*.o
	rm -rf drivers/*.o
	rm -rf utils/*.o
