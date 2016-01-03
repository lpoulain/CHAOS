; Routines to access BIOS resources (screen, disk)


; Prints a text string to the screen
print_string:
    mov ah, 0Eh     ; int 10h 'print char' function

.repeat:
    lodsb           ; Get character from string
    cmp al, 0
    je .done        ; If char is zero, end of string
    int 10h         ; Otherwise, print it
    jmp .repeat

.done:
    ret


print_hex :
	mov bx , HEX_OUT 	; print the string pointed to
	call print_string 	; by BX
	ret


; Reads [dh] sectors from disk
; Starting at sector 2
read_from_disk:
	push dx				; We will use dx, so let's save it to the stack

	mov ah, 0x02		; BIOS read sector function
	mov al, dh			; Reads DH sectors
;	mov ch, 0			; Select cylinder 0 (actually set by the calling program)
	mov dh, 0 			; Select head 0
	mov cl, 2 			; Start at the second sector (right after the boot sector)

	int 0x13 			; Execute the BIOS function

	jc error 			; Jumps to :error if the error flag is set

	pop dx
	cmp dh, al			; Compares the number of sectors actually read
						; with the number we want to read
	jne error 			; Jumps to :error if the two are different

	ret

error:
	mov si, BIOS_DISK_ERROR_MSG
	call print_string
	jmp $

; Global variables
BIOS_DISK_ERROR_MSG: db "Error reading disk", 13, 10, 0
HEX_OUT : db '0x0000' ,0
