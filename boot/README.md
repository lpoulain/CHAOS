## Bootloader

This directory contains the elements for the initial bootloader. I have since switched to GRUB, but keep these files as a reference. The bootloader reads the kernel from the disk, loads it in memory at address 0x1000, switches to 32-bit protected mode and executes that kernel. All this must fit within 512 bytes to fit in the boot sector.

Here are the files:

- boot.asm: the boot main file
- bios.asm: the routines that handle the BIOS I/O functions (printing on the screen, reading from disk)
- pm_start.asm: the code that switches to protected mode
- pm_gdt.asm: the definitions for the GDT (to switch to protected mode)
- pm_io.asm: once in protected mode, the BIOS functions are not available anymore so I/O must be done differently

This bootloader does the bare minimum to launch the kernel. It does not support any filesystem - it expects the kernel to be at a precise location n the disk starting on the second sector and loads a hard-coded number of sectors. It only supports a binary executable format (no ELF binary).

Last but not least, it supports a limited kernel size. Beyond a certain size, I had to break the kernel into two BIOS calls to have it work on both bochs and VirtualBox.
