; MBR (Master Boot Record)

; CPU starts in 16 bit real mode. BIOS initializes hardware, looks for bootable disk (512 bytes end with 0x55 0xAA)
; BIOS then copies 512 bytes to 0x7C00 and jumpts to it, leaving drive number in DL register

[bits 16] ; tell NASM (Netwide Assembler) to emit 16 bit instruction encodings
[org 0x7c00] ; tell NASM when code runs, it will be at address 0x7C00

STAGE2_LBA equ 1 ; LBA (logical block address) 1 for bootloader stage 2 (stage 1 is 0)
STAGE2_ADDR equ 0x7e00 ; 0x7C00 + 512 = 0x7E00 (LBA = 512)
STAGE2_COUNT equ 8 ; Stage 2 is 8 sectors (4096 bytes)

; Load step 2
start:
    cli ; disable interupts
    xor ax, ax ; ax = 0 (primary accumulator register for math/logic/data movement)
    mov ds, ax ; ds = 0 (data segment for variables/data)
    mov es, ax ; es = 0 (extra segment, like ds)
    mov ss, ax ; ss = 0 (stack segment, start of the stack)
    mov sp, 0x7c00 ; stack pointer (current top of the stack)
    sti ; re-enable interupts

    mov [boot_drive], dl ; Save the BIOS (Basic Input Output System) drive number 
    mov si, msg1 ; print message 1
    call print

    mov byte [retries], 5 ; will retry this 5 times
.rtry:
    mov si, dap ; Disk address packet
    mov ah, 0x42 ; Extended read
    mov dl, [boot_drive] ; Go to where we want to boot
    int 0x13 ; BIOS disk service
    jnc .rok ; jump if it worked
    xor ah, ah ; reset disk
    mov dl, [boot_drive] ; Go to where we want to boot
    int 0x13 ; BIOS disk service
    dec byte [retries] ; Decrement a try
    jnz .rtry ; Loop while retries remain
    mov si, msg_err ; Print the error
    call print
.hb:
    hlt ; If it didn't work halt
    jmp .hb
.rok:
    mov dl, [boot_drive] ; If it worked reload DL
    jmp STAGE2_ADDR ; Jump to boot stage 2

; Printing function
print:
    ; Push these to the stack
    push ax
    push bx
    push dx
.n:
    lodsb ; load string byte
    test al, al ; compute al & al and sets flags storing the results
    jz .d ; byte was 0
    mov bl, al ; Stash t in bl
.w:
    ; Read the line status register, and spin
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz .w
    mov al, bl
    mov dx, 0x3f8
    out dx, al
    jmp .n
.d:
    ; Remove these from the stack
    pop dx
    pop bx
    pop ax
    ret

; Basic debug + variables
boot_drive: db 0
retries: db 0
msg1: db "[boot] Stage 1: reading stage 2 from disk", 13, 10, 0
msg_err: db "[boot] Stage 1: DISK ERROR", 13, 10, 0

dap:
    db 0x10 ; size of packet is 16 bytes
    db 0 ; reserved
    dw STAGE2_COUNT ; sectors to read
    dw STAGE2_ADDR ; destination of offset
    dw 0 ; destinatio sement
    dq STAGE2_LBA ; destination LBA

times 510-($-$$) db 0 ; Padding
dw 0xaa55 ; This needs to be at the end of the MBR