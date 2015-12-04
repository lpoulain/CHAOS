; Boot sector
; Boots the operating system in 16 mode

[org 0x7c00]                ; Location of the code in memory

KERNEL_ADDRESS equ 0x1000

mov ax, 0x0
mov es, ax

start:
;    mov ax, 07C0h           ; Set up 4K stack space after this bootloader
;    add ax, 288             ; (4096 + 512) / 16 bytes per paragraph
;    mov ss, ax
;    mov sp, 4096
;
;    mov ax, 07C0h           ; Set data segment to where we're loaded
;    mov ds, ax


    mov [BOOT_DRIVE], dl    ; The BIOS stores the boot drive in dl
                            ; so let's copy that value to BOOT_DRIVE

    mov bp, 0x9000          ; Sets the stack to 0x9000
    mov sp, bp              ; sp = Stack Pointer

    ; Prints the welcome message
    mov si, WELCOME_MSG     ; Put string position into SI
    call print_string       ; Call our string-printing routine

    ; Loads the kernel in memory
    mov dh, 0x20            ; Loads 16 sectors
    mov dl, [BOOT_DRIVE]    ; From drive [BOOT_DRIVE]
    mov bx, KERNEL_ADDRESS  ; And stores them at 0x1000
    call read_from_disk

;    mov si, 0x1000          ; Prints the content on the screen
;    call print_string

;    mov dx, 0x1000
;    call print_hex

    ; Switches to protected mode
    call switch_to_protected_mode

    ; End
    jmp $                   ; Infinite loop (end of the program)


%include "boot/bios.asm"
%include "boot/pm_gdt.asm"
%include "boot/pm_io.asm"
%include "boot/pm_start.asm"


; Global variables 
    BOOT_DRIVE: db 0
    WELCOME_MSG: db 'Welcome to Laurent OS!', 10,13, 0

    times 510-($-$$) db 0   ; Pad remainder of boot sector with 0s
    dw 0xAA55               ; The standard PC boot signature
 