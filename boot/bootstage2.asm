[bits 16]
[org 0x7e00]

; Segment selector is byte offset into GDT (Global descriptor table)
CODE_SEG equ gdt_code - gdt_start 
DATA_SEG equ gdt_data - gdt_start
CODE64_SEG equ gdt64_code - gdt64_start
DATA64_SEG equ gdt64_data - gdt64_start

BOOT_INFO equ 0x4000 ; Struct for C kernel
MMAP_BUFFER equ 0x5000 ; Array of E820 entries
KERNEL_LBA equ 9 ; Sector 9 is after MBR + stage 2
ELF_BUFFER equ 0x10000 ; Where raw ELF lands
KERNEL_SECTORS equ 100 ; read 51,200 bytes

stage2_start:
    cld ; clear direction flag
    xor ax, ax ; ax = 0
    mov es, ax ; es = 0
    mov [boot_drive], dl ; same as before

    mov si, s_load ; Print debug message
    call print16
    call load_kernel

    mov si, s_mmap
    call print16
    call do_e820 ; ask BIOS where RAM is

    ; fill the boot_info structure
    movzx eax, word [mmap_count_var]
    mov [BOOT_INFO + 0], eax ; mmap_count
    movzx eax, byte [boot_drive]
    mov [BOOT_INFO + 4], eax ; boot_drive
    mov dword [BOOT_INFO + 8],  MMAP_BUFFER
    mov dword [BOOT_INFO + 12], 0 ; mmap_addr (u64)
    mov dword [BOOT_INFO + 16], 0
    mov dword [BOOT_INFO + 20], 0 ; acpi_rsdp reserved (filled later)

    mov si, s_a20 ; Print debug message
    call print16
    call enable_a20 ; Get A20 working
    call verify_a20

    ; Jump into protected mode
    cli ; Turn off interupts
    lgdt [gdt_descriptor] ; Load GDT
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:protected_mode ; PE bit, far jump

; load kernel ELF
load_kernel:
    mov byte [retries], 5
.try:
    mov si, dap_kernel ; identical to stage 1 with dap_kernel
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .ok
    xor ah, ah
    mov dl, [boot_drive]
    int 0x13
    dec byte [retries]
    jnz .try
    mov si, s_kerr
    call print16
    jmp halt16
.ok:
    ret

; BIOS E820 memory map
do_e820:
    mov di, MMAP_BUFFER
    xor ebx, ebx
    xor bp, bp
    mov edx, 0x534D4150 ; Continuation value
    mov eax, 0xE820 ; ASCII SMAP signature
    mov dword [es:di + 20], 1
    mov ecx, 24 ; 24 byte entry/buffer offering
    int 0x15
    jc .fail ; If it fails try without E820
    mov edx, 0x534D4150
    cmp eax, edx
    jne .fail
    test ebx, ebx
    je .fail
    jmp .have
.next: ; Reload APCI EAX ECX every iteration
    mov eax, 0xE820
    mov dword [es:di + 20], 1 ; ACPI 3.0 attributes
    mov ecx, 24
    int 0x15
    jc .done
    mov edx, 0x534D4150
.have:
    jcxz .skip ; Jump if cx is 0
    mov ecx, [es:di + 8] 
    or  ecx, [es:di + 12]
    jz .skip
    inc bp
    add di, 24
.skip: ; Loop while EBX is non-zero
    test ebx, ebx
    jne .next
.done:
    mov [mmap_count_var], bp
    ret
.fail:
    mov word [mmap_count_var], 0
    ret

enable_a20: ; Enable A20 cause IBM messed something up
    in al, 0x92
    or al, 2
    and al, 0xfe
    out 0x92, al
    ret

; Verify A20 by testing the 1MB wraparound
verify_a20:
    push ds
    push es
    xor ax, ax
    mov ds, ax
    mov ax, 0xffff
    mov es, ax
    mov byte [ds:0x0500], 0x00
    mov byte [es:0x0510], 0xff ; = phys 0x100500 if A20 on, else aliases 0x0500
    mov al, [ds:0x0500]
    pop es
    pop ds
    cmp al, 0xff
    je .off
    ret
.off:
    mov si, s_a20err
    call print16
    jmp halt16

; Halt but now we are in 16s
halt16:
    hlt
    jmp halt16

; Print but now we are in 16s. Same as before
print16:
    push ax
    push bx
    push dx
.n:
    lodsb
    test al, al
    jz .d
    mov bl, al
.w:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz .w
    mov al, bl
    mov dx, 0x3f8
    out dx, al
    jmp .n
.d:
    pop dx
    pop bx
    pop ax
    ret

; Now we build the pages table
[bits 32]
protected_mode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    mov esi, s_prot
    call print32

    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, (3*4096)/4
    rep stosd
    mov dword [0x1000], 0x2000 | 3 ; Paging pml4 -> pdpt
    mov dword [0x2000], 0x3000 | 3 ; pdpt -> pd 0> pt
    mov edi, 0x3000
    mov eax, 0x83
    mov ecx, 512
.fill_pd: ; Page size, 1 GiB idenity mapped
    mov [edi], eax
    add edi, 8
    add eax, 0x200000
    dec ecx
    jnz .fill_pd

    mov eax, cr4 ; CR4 bit 5 PAE (physical address extension)
    or eax, 1 << 5
    mov cr4, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    mov eax, cr0 ; MSR is Model specific registers. EFER and LME
    or eax, 1 << 31
    mov cr0, eax

    mov esi, s_page ; pg paging
    call print32 

    lgdt [gdt64_descriptor]
    jmp CODE64_SEG:long_mode ; 64 bit GDT

; Same print but now for 32s
print32:
    push eax
    push ebx
    push edx
.n:
    lodsb
    test al, al
    jz .d
    mov bl, al
.w:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz .w
    mov al, bl
    mov dx, 0x3f8
    out dx, al
    jmp .n
.d:
    pop edx
    pop ebx
    pop eax
    ret

; Now we get to 64. 
[bits 64]
long_mode: ; Parse and load the ELF
    mov ax, DATA64_SEG ; has sections for linkers, segments for headers
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000
    cld

    mov rsi, s_long
    call print64
    mov rsi, s_elf
    call print64

    mov rbx, ELF_BUFFER
    mov r12, [rbx + 0x20]
    add r12, rbx
    movzx r13, word [rbx + 0x38]
    movzx r14, word [rbx + 0x36]
.ph_loop:
    mov eax, [r12]
    cmp eax, 1
    jne .skip
    mov rsi, s_seg
    call print64
    mov rax, [r12 + 0x10]
    call print_hex64
    mov rsi, s_nl
    call print64
    mov rsi, [r12 + 0x08]
    add rsi, rbx
    mov rdi, [r12 + 0x10]
    mov rcx, [r12 + 0x20]
    rep movsb
    mov rcx, [r12 + 0x28]
    sub rcx, [r12 + 0x20]
    xor al, al ; AMD64 System V handels /bss differently
    rep stosb
.skip:
    add r12, r14
    dec r13
    jnz .ph_loop

    mov rax, [rbx + 0x18] ; e_entry
    mov rdi, BOOT_INFO ; first argument to kmain
    jmp rax

putc64: ; Put things in 64
    push rdx
    push rbx
    mov bl, al
.w:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz .w
    mov al, bl
    mov dx, 0x3f8
    out dx, al
    pop rbx
    pop rdx
    ret
print64: ; Printing in 64
    push rax
    push rsi
.n:
    lodsb
    test al, al
    jz .d
    call putc64
    jmp .n
.d:
    pop rsi
    pop rax
    ret
print_hex64: ; Same thing
    push rax
    push rbx
    push rcx
    mov rbx, rax
    mov al, '0'
    call putc64
    mov al, 'x'
    call putc64
    mov rcx, 16
.l:
    rol rbx, 4
    mov al, bl
    and al, 0x0f
    cmp al, 10
    jb .num
    add al, 'a' - 10
    jmp .out
.num:
    add al, '0'
.out:
    call putc64
    dec rcx
    jnz .l
    pop rcx
    pop rbx
    pop rax
    ret

; Debug messages
boot_drive: db 0
retries: db 0
mmap_count_var: dw 0
s_load: db "[boot] stage 2: loading kernel image", 13, 10, 0
s_mmap: db "[boot] stage 2: querying memory map (E820)", 13, 10, 0
s_a20: db "[boot] stage 2: enabling + verifying A20", 13, 10, 0
s_prot: db "[boot] stage 2: 32-bit protected mode", 13, 10, 0
s_page: db "[boot] stage 2: paging on, first 1 GiB mapped", 13, 10, 0
s_long: db "[boot] stage 2: 64-bit long mode", 13, 10, 0
s_elf: db "[boot] stage 2: loading ELF kernel", 13, 10, 0
s_seg: db "[boot] load segment -> ", 0
s_entry: db "[boot] kernel entry  -> ", 0
s_nl: db 13, 10, 0
s_kerr: db "[boot] stage 2: KERNEL LOAD ERROR", 13, 10, 0
s_a20err: db "[boot] stage 2: A20 FAILED", 13, 10, 0

; Finishing stuff
dap_kernel:
    db 0x10
    db 0
    dw KERNEL_SECTORS
    dw 0x0000
    dw 0x1000
    dq KERNEL_LBA

; GDT information
gdt_start:
    dq 0
gdt_code:
    dw 0xffff
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0
gdt_data:
    dw 0xffff
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt64_start:
    dq 0
gdt64_code:
    dq 0x00209A0000000000
gdt64_data:
    dq 0x0000920000000000
gdt64_end:
gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

times 4096-($-$$) db 0 ; Padding