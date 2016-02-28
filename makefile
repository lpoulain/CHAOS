SDIR = kernel lib drivers utils fs
C_SOURCES = $(wildcard kernel/*.c lib/*.c drivers/*.c utils/*.c gui/*.c fs/*.c)
HEADERS = $(wildcard kernel/*.h lib/*.h drivers/*.h utils/*.h gui/*.h fs/*.h)
OBJ = ${C_SOURCES:.c=.o}
INCLUDE= -I ./kernel -I ./lib -I ./drivers -I ./utils -I ./gui -I ./fs

All: chaos.img

chaos.img:	kernel.elf kernel_v.elf kernel.sym kernel_v.sym
	cp kernel.elf /Volumes/CHAOS/
	cp kernel_v.elf /Volumes/CHAOS/
	cp kernel.sym /Volumes/CHAOS/
	cp kernel_v.sym /Volumes/CHAOS/

bin:	boot.bin kernel.bin
	dd if=/dev/zero of=chaos0.img ibs=1k count=1440
	dd if=boot.bin of=chaos0.img conv=notrunc
	dd if=kernel.bin of=chaos0.img seek=1 conv=notrunc	

boot.bin:	boot/boot.asm boot/bios.asm boot/pm_gdt.asm boot/pm_start.asm boot/pm_io.asm
	nasm -f bin -o boot.bin boot/boot.asm

%.o : %.c ${HEADERS}
	/usr/local/bin/i686-elf-gcc-5.3.0 -std=gnu99 -m32 -ffreestanding $(INCLUDE) -g -c $< -o $@

kernel/hal.o: kernel/hal.asm
	nasm kernel/hal.asm -f elf32 -o kernel/hal.o

kernel/main_vga.o: kernel/main_vga.asm
	nasm kernel/main_vga.asm -f elf32 -o kernel/main_vga.o

kernel/main_text.o: kernel/main_text.asm
	nasm kernel/main_text.asm -f elf32 -o kernel/main_text.o

kernel.bin: kernel/kernel_entry.o ${OBJ}
	/usr/local/i686-elf/bin/ld -Tlink.ld -m elf_i386 -o kernel.bin -Ttext 0x1000 $^ --oformat binary -Map kernel.map

kernel.elf: kernel/main_text.o kernel/hal.o ${OBJ}
	/usr/local/i686-elf/bin/ld -Tlink.ld -m elf_i386 -o kernel.elf $^ -Map kernel.map

kernel_v.elf: kernel/main_vga.o kernel/hal.o ${OBJ}
	/usr/local/i686-elf/bin/ld -Tlink.ld -m elf_i386 -o kernel_v.elf $^ -Map kernel_v.map

kernel.sym: kernel.elf
	./objcopy --only-keep-debug kernel.elf kernel.sym

kernel_v.sym: kernel_v.elf
	./objcopy --only-keep-debug kernel_v.elf kernel_v.sym
	./objdump -g kernel_v.sym > symbols.txt >& /dev/null

echo: utils/echo/echo.c
	/usr/local/bin/i686-elf-gcc-5.3.0 -std=gnu99 -m32 -ffreestanding -fno-asynchronous-unwind-tables $(INCLUDE) -g -c $< -o utils/echo/echo.o
	/usr/local/i686-elf/bin/ld -Tlink.ld -m elf_i386 -q -o utils/echo/echo utils/echo/echo.o
	cp utils/echo/echo /Volumes/CHAOS/

formula: utils/formula/formula.c
	/usr/local/bin/i686-elf-gcc-5.3.0 -std=gnu99 -m32 -ffreestanding -fno-asynchronous-unwind-tables $(INCLUDE) -g -c $< -o utils/formula/formula.o
	/usr/local/i686-elf/bin/ld -Tlink.ld -m elf_i386 -q -o utils/formula/formula utils/formula/formula.o
	cp utils/formula/formula /Volumes/CHAOS/

clean:
	rm -rf boot.bin
	rm -rf kernel.bin
	rm -rf kernel.elf
	rm -rf kernel/*.o
	rm -rf lib/*.o
	rm -rf drivers/*.o
	rm -rf utils/*.o
	rm -rf fs/*.o
