## CHAOS (CHeers, Another Operating System)

CHAOS is a simple 32-bit multitasking operating system I am writing from scratch in assembly and C in order to better understand how operating systems work.

So far, here is what it does:

#### Bootloader

The initial bootloader was initially hand-written. But it started to show its limits beyond a certain point, so I decided to switch to GRUB instead of writing my own 2-stage bootloader. The switch to GRUB allowed the following:

- Load a larger kernel: beyond a certain size, just reading from the floppy started to get unreliable
- Use a filesystem (instead of storing the kernel at a precise location on the disk). The current chaos.img is a FAT12 floppy disk image
- Use an ELF binary with symbols. This allows to run commands such as addr2line or objdump for better debugging

The initial bootloader is still in the boot directory, even if it's not used anymore.

#### The kernel

The kernel sets up several things:

- A "device driver" to handle displaying on the screen
- Heap: a very primitive heap mechanism (i.e. malloc())
- Interrupts: This is used to capture keystrokes. Note that the keyboard handler is merely storing the keystroke in the process buffer
- Processes: each process has its own display (i.e. a window on the screen), stack, etc.
- Paging. This allows to map the virtual memory to the physical memory as the operating system sees fits. Right now, trying to access an unmapped page results in a page fault, resulting in the OS mapping that virtual page to a "forbidden page" which is filled with 0xFFFFFFFF. Each process has their own virtual memory mapping.
- Multitasking: the interrupts are also used to have a scheduler function called at regular intervals. This allows to perform context switches automatically.

#### The processes

The operating system launches two processes, each one represented by a shell in its own window (type "help to see the available commands"). Those processes are run concurrently and can perform tasks in parallel (this is the purpose of the "countdown" command). Press the Tab key to switch from one shell to the other.

When the processes are waiting for a keyboard input, they are in polling mode, which means they are waiting for the keyboard handler to store any keystroke in their buffer. A process in polling mode is not being given cycles by the scheduler.

#### How to run it

The makefile is designed to work with the GNU's gcc and ld. The build can happen on any platform as long as it's a version of gcc which can generate ELF binaries. On OS X, Macports' version of gcc and ld only generate Mach-O executable, so you will need to manually download and build binutils and gcc (see [http://wiki.osdev.org/GCC_Cross-Compiler])

The resulting image is chaos.img, which is a floppy disk image. You can run it using the x86 emulator [bochs](http://bochs.sourceforge.net/) (once installed just type 'bochs', the bochrc file from the project points it to the right image) or on VirtualBox by specifying chaos.img as the boot floppy.

#### Thanks

This project has been possible thanks to the numerous resources available on the Internet:

- [Writing a Simple Operating System from Scratch](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
- [BSOS](https://github.com/aplabs/bsos)
- [Operating System Development Series](http://www.brokenthorn.com/Resources/OSDevIndex.html)
- [Bran's Kernel Development Tutorial](http://www.osdever.net/bkerndev/Docs/intro.htm)
- [JamesM's kernel development tutorials](http://www.jamesmolloy.co.uk/tutorial_html/)
