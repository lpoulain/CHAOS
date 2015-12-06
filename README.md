## CHAOS (CHrist, Another Operating System)

CHAOS is a simple operating system I am writing from scratch in assembly and C in order to better understand how operating systems work.

So far, here is what it does:

- The boot sector reads the kernel from disk and loads it into memory
- It switches to the i386 32-bit protected mode
- It runs the kernel code loaded into memory
- That kernel has two "device drivers": one for the screen, one for the keyboard
- It also sets up interruptions in order to be able to catch keystrokes
- It launches two basic shells which have each two commands: cls and help
- You switch from one shell to another using the tab key

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
