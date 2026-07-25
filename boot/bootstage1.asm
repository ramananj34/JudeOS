[bits 16]
[org 0x7c00]

STAGE2_LBA equ 1 ; stage 2 begins at sector 1 (sector 0 = this boot sector)
STAGE2_ADDR equ 0x7e00 ; load stage 2 right above us
STAGE2_COUNT equ 1 ; number of sectors to read

start:
    cli ; no interrupts while we set up the stack
    xor ax, ax
    mov ds, ax ; data  segment = 0
    mov es, ax ; extra segment = 0
    mov ss, ax ; stack segment = 0
    mov sp, 0x7c00 ; stack grows down from just below our code
    sti

    mov [boot_drive], dl ; BIOS passes the boot drive number in dl; save it
    mov si, msg_boot
    call print

    mov si, dap ; ds:si -> Disk Address Packet
    mov ah, 0x42 ; BIOS extended read (LBA)
    mov dl, [boot_drive]
    int 0x13
    jc disk_error ; carry flag set => read failed

    jmp STAGE2_ADDR ; hand control to stage 2

disk_error:
    mov si, msg_err
    call print
.hang:
    hlt
    jmp .hang

; print a NUL-terminated string at ds:si over the COM1 serial port
print:
    push ax
    push bx
    push dx
.next:
    lodsb ; al = [ds:si], si++
    test al, al
    jz .done
    mov bl, al
.wait:
    mov dx, 0x3fd ; COM1 Line Status Register
    in al, dx
    test al, 0x20 ; bit 5 = transmit register empty?
    jz .wait
    mov al, bl
    mov dx, 0x3f8 ; COM1 data register
    out dx, al
    jmp .next
.done:
    pop dx
    pop bx
    pop ax
    ret

boot_drive: db 0
msg_boot: db "Stage 1: booting...", 13, 10, 0
msg_err: db "Stage 1: DISK ERROR", 13, 10, 0

dap:
    db 0x10 ; packet size
    db 0 ; reserved
    dw STAGE2_COUNT ; sectors to read
    dw STAGE2_ADDR ; destination offset
    dw 0 ; destination segment
    dq STAGE2_LBA ; starting sector (LBA)

times 510-($-$$) db 0 ; pad to byte 510
dw 0xaa55 ; boot signature