## CHAOS (CHrist, Another Operating System)

CHAOS is a simple operating system I am writing in C in order to better understand how operating systems work.

So far, here is what it does:

- The bootstrap reads the kernel from disk and loads it into memory
- It switches to the i386 32-bit protected mode
- It runs the kernel, which has two "device drivers": one for the screen, one for the keyboard
- The kernel sets up interruptions in order to be able to catch keystrokes
- It launches a basic shell with two commands: cls and help

This project has been possible thanks to the numerous resources available on the Internet:

- [Writing a Simple Operating System from Scratch](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
- [BSOS](https://github.com/aplabs/bsos)
- [Operating System Development Series](http://www.brokenthorn.com/Resources/OSDevIndex.html)
- [Bran's Kernel Development Tutorial](http://www.osdever.net/bkerndev/Docs/intro.htm)
- [JamesM's kernel development tutorials](http://www.jamesmolloy.co.uk/tutorial_html/)
