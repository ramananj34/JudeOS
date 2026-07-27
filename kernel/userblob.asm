[bits 64]
section .rodata
global user_blob
global user_blob_end
user_blob:
    incbin "build/user.elf"
user_blob_end: