[bits 16] ; 16-bit real mode instructions
[org 0x7c00] ; BIOS loads us at physical address 0x7C00

start:
    cli ; disable interrupts while we touch the stack
    xor ax, ax ; ax = 0
    mov ds, ax ; data segment  = 0
    mov es, ax ; extra segment = 0
    mov ss, ax ; stack segment = 0
    mov sp, 0x7c00 ; stack grows down from just below our code
    sti ; interrupts back on
    mov si, msg ; si points at our string

.print_loop:
    lodsb ; load byte at [ds:si] into al, then si = si + 1
    test al, al ; is the byte zero (end of string)?
    jz .hang ; if so, we're done
    mov bl, al ; stash the character in bl
.wait_tx:
    mov dx, 0x3fd ; COM1 Line Status Register
    in al, dx ; read it
    test al, 0x20 ; bit 5 set means "ready for a byte"
    jz .wait_tx ; not ready yet -> keep polling
    mov al, bl ; get our character back
    mov dx, 0x3f8 ; COM1 data register
    out dx, al ; transmit the byte
    jmp .print_loop ; next character

.hang:
    hlt ; sleep the CPU
    jmp .hang ; forever

msg db "Hello from my own boot sector", 13, 10, 0   ; 13,10 = \r\n, 0 = end

times 510-($-$$) db 0 ; pad with zeros out to byte 510
dw 0xaa55 ; the boot signature at bytes 510-511