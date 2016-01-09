/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */

PHDRS
{
    debug_info PT_LOAD;
}

SECTIONS
{
    /* Begin putting sections at 1 MiB, a conventional place for kernels to be
       loaded at by the bootloader. */
    . = 1M;

    /* First put the multiboot header, as it is required to be put very early
       early in the image or the bootloader won't recognize the file format.
       Next we'll put the .text section. */
    .text BLOCK(4K) : ALIGN(4K)
    {
        code = .; _code = .; __code = .;
        *(.multiboot)
        *(.text)
    }

    /* Read-only data. */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        rodata = .; _rodata = .; __rodata = .;
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K)
    {
        data = .; _data = .; __data = .;
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        bss = .; _bss = .; __bss = .;
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }

    /* The compiler may produce other sections, by default it will put them in
       a segment with the same name. Simply add stuff here as needed. */
/*
    .stab BLOCK(4K) : ALIGN(4K)
    {
        stab = .;
        *(.symtab)
    }

    .stabstr BLOCK(4K) : ALIGN(4K)
    {
        stabstr = .;
        *(.stabstr)
    }
*/
    .debug_info BLOCK(4K) : ALIGN(4K)
    {
        debug_info = .;
        *(.debug_info)
    } :debug_info

    end = .; _end = .; __end = .;
}
