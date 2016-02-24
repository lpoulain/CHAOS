# Bootloader

This directory contains the elements for the initial bootloader which is not used anymore, but is kept as a reference. The latest versions of the OS use GRUB then GRUB 2 as a bootloader. The switch to GRUB allowed the following:

- Load a larger kernel: beyond a certain size, just reading from the floppy started to get unreliable
- Use a filesystem (instead of storing the kernel at a precise location on the disk). The current chaos.img is a FAT12 hard disk image (formerly floppy disk)
- Use an ELF binary with symbols. This allows to run commands such as addr2line or objdump for better debugging
- The switch from GRUB to GRUB 2 allowed to boot in graphic mode. Doing it with a custom bootloader is easy, but GRUB 2 has a better support than GRUB in that regard


## The original bootloader

The initial bootloader was initially hand-written. It reads the kernel from the floppy disk (at a particular location), loads it into memory at address 0x1000, switches to 32-bit protected mode and executes that kernel. All this must fit within 512 bytes to fit in the boot sector.

Here are the files:

- boot.asm: the boot main file
- bios.asm: the routines that handle the BIOS I/O functions (printing on the screen, reading from disk)
- pm_start.asm: the code that switches to protected mode
- pm_gdt.asm: the definitions for the GDT (to switch to protected mode)
- pm_io.asm: once in protected mode, the BIOS functions are not available anymore so I/O must be done differently

This bootloader does the bare minimum to launch the kernel. It does not support any filesystem - it expects the kernel to be at a precise location on the disk starting on the second sector and loads a hard-coded number of sectors. It only supports a binary executable format (no ELF binary).

Last but not least, it supports a limited kernel size. Beyond a certain size, I had to break the kernel into two BIOS calls to have it work on both bochs and VirtualBox.
