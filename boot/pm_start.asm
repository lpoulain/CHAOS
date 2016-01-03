[bits 16]

switch_to_protected_mode:
	cli							; Turns off interruptions
	lgdt [ gdt_descriptor ] 	; Load our global descriptor table , which defines
								; the protected mode segments ( e.g. for code and data )
	mov eax , cr0 				; To make the switch to protected mode , we set
	or eax , 0x1 				; the first bit of CR0 , a control register
	mov cr0 , eax
	jmp CODE_SEG : init_pm 		; Make a far jump ( i.e. to a new segment ) to our 32 - bit
								; code. This also forces the CPU to flush its cache of
								; pre - fetched and real - mode decoded instructions , which can
								; cause problems.

[bits 32]
; Initialise registers and the stack once in PM.
init_pm :
	mov ax , DATA_SEG 			; Now in PM , our old segments are meaningless ,
	mov ds , ax 				; so we point our segment registers to the
	mov ss , ax 				; data selector we defined in our GDT
	mov es , ax
	mov fs , ax
	mov gs , ax
	mov ebp , 0x90000 			; Update our stack position so it is right
	mov esp , ebp 				; at the top of the free space.
	call pm_started				; Finally , call some well - known label

pm_started:
    call clear_screen_pm
    mov ebx, PROTECTED_MODE_MSG
    call print_string_pm
	call KERNEL_ADDRESS

    jmp $

; Global variables 
    PROTECTED_MODE_MSG: db 'Welcome to CHAOS (CHeers, Another Operating System)!', 0	
