[bits 16]
[org 0x7e00]

start:
    mov si, msg
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
    lodsb
    test al, al
    jz .done
    mov bl, al
.wait:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz .wait
    mov al, bl
    mov dx, 0x3f8
    out dx, al
    jmp .next
.done:
    pop dx
    pop bx
    pop ax
    ret

msg: db "Stage 2: booting...", 13, 10, 0

times 512-($-$$) db 0 ; pad stage 2 out to a full 512-byte sector